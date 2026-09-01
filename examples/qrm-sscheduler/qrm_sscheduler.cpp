/*
 * Copyright 2024 - 2026 Munich Quantum Software Stack
 * All rights reserved.
 *
 * Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// A gRPC ingress example built on Scheduler (include/scheduler/scheduler.hpp).
//
// Architecture follows QRM's own deployment/workflow-aio/main.cpp
// (https://github.com/Munich-Quantum-Software-Stack/QRM/blob/qrm-ci/
// deployment/workflow-aio/main.cpp#L23): a scheduler with a background
// thread that owns the QDMI submission path, continuously pulling ready
// tasks off the scheduler and submitting them, decoupled from however tasks
// arrive. QRM runs that whole thing as two std::threads in one process;
// here it's adapted to an MPI rank-per-role split:
//
//   rank 0: gRPC listener (mqss_grpc_server.cpp) — builds/serializes
//           mqss::scheduler::QuantumTask (Scheduler's task type).
//   rank 1: Scheduler<mqss::scheduler::QuantumTask> — the MPI_Recv loop below only
//           ever calls scheduleTask(); a separate std::thread inside this
//           same rank owns the QDMI session + Submitter and calls
//           getNextReadyTask() in a loop, submitting whatever comes back.
//
// One QDMI device, submitted to from one thread — no per-alias fan-out to
// further MPI ranks, since demonstrating Scheduler's own queuing/dispatch
// behavior is the point here, not multi-device concurrency. QcAlias-based
// routing is consequently also not wired up; mqss::scheduler::QuantumTask carries
// preferred_qpu/scheduled_qpu (mirroring the real protobuf schema) but
// nothing here reads them yet.

#include <mpi.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <qinfo/qinfo.h>
#include <qdmi.hpp>
#include <qdmi/constants.h>

// The older, LLVM/JIT-coupled QuantumTask struct + Submitter class -
// Submitter::submitTask still expects this type, not Scheduler's
// mqss::scheduler::QuantumTask (see scheduler/quantum_task.hpp's header comment).
// Distinguished from mqss::scheduler::QuantumTask below by namespace and include path.
#include <submitter.hpp>
#include <quantum_task.hpp>

#include <scheduler/scheduler.hpp>
#include <scheduler/quantum_task.hpp>

#include "mqss_grpc_server.hpp"

// --------------------------------------------------------------
// Graceful shutdown on Ctrl+C (SIGINT) / SIGTERM. mpirun forwards the
// signal to each rank process individually (verified empirically — they
// run in separate process groups from mpirun's own, so this isn't raw
// terminal process-group delivery). A first attempt handled it with a
// traditional signal(2) handler that called server->Shutdown() directly -
// this crashed: the handler runs on whichever thread the signal
// interrupts, which for rank 0 is the same thread blocked inside
// server->Wait(), and Shutdown() taking the same internal mutex Wait() is
// already (in effect) held up on is a self-deadlock; abseil's Mutex
// detects the reentrant lock and abort()s rather than hanging silently.
// Shutdown() is not async-signal-safe, so it cannot be called from a
// signal handler that might interrupt the very thread that's inside
// Wait().
//
// The fix: block SIGINT/SIGTERM on every thread (done once, right after
// MPI_Init — new threads inherit the mask), and have rank 0 spawn a
// dedicated watcher thread that sigwait()s for them. sigwait() only
// consumes an already-pending, already-blocked signal — no handler, no
// interrupted-thread reentrancy — so calling Shutdown() from inside it is
// safe. Rank 1 never calls sigwait() itself, so SIGINT/SIGTERM just stay
// blocked-and-pending on it, which is enough on its own to stop the
// default terminate-on-signal action from killing it before rank 0's
// shutdown sentinel (below) reaches it over the existing MPI pipe.
//
// Note: as of this writing, sending SIGINT/SIGTERM to the mpirun process
// externally (e.g. `kill -TERM <mpirun-pid>`) has been observed to not
// reliably reach rank 0's sigwait() at all in this environment — ranks end
// up killed by mpirun's own --mca odls_base_sigkill_timeout instead. Not
// yet root-caused; an actual interactive Ctrl+C may behave differently.
// --------------------------------------------------------------

// Converts Scheduler's lightweight mqss::scheduler::QuantumTask into the older,
// Submitter-compatible QuantumTask struct - the one adapter point where
// the two task types meet, kept right next to where it's used (the
// submitter thread below) rather than in a shared header, since nothing
// else in this pipeline needs it.
std::shared_ptr<QuantumTask> to_submitter_task(const mqss::scheduler::QuantumTask &task) {
    auto old_task = std::make_shared<QuantumTask>(task.task_id());
    old_task->NumberQbits = task.n_qbits();
    old_task->NumberShots = task.n_shots();
    old_task->CircuitFile = task.circuit_files().empty() ? "" : task.circuit_files().front();
    old_task->CircuitFileType = task.circuit_file_type();
    old_task->ResultsDestination = task.result_destination();
    old_task->SubmitTime = time(nullptr);
    return old_task;
}

int main(int argc, char **argv) {
    // Line-buffer stdout even when redirected to a file (the default there
    // is fully-buffered) — this output is this whole pipeline's log, so a
    // crash or a fast SIGKILL (which, unlike SIGINT/SIGTERM, can't be
    // blocked or caught to flush first) shouldn't be able to silently
    // swallow the last few lines written just before it.
    setvbuf(stdout, nullptr, _IOLBF, 0);

    int rank, size, provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    if (provided < MPI_THREAD_SERIALIZED) {
        std::cerr << "This MPI implementation does not support "
                     "MPI_THREAD_SERIALIZED, required for the gRPC listener "
                     "rank.\n";
        MPI_Finalize();
        return 1;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    sigset_t shutdown_signals;
    sigemptyset(&shutdown_signals);
    sigaddset(&shutdown_signals, SIGINT);
    sigaddset(&shutdown_signals, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &shutdown_signals, nullptr);

    if (size < 2) {
        if (rank == 0) {
            std::cerr << "Need at least 2 MPI processes (1 gRPC listener + "
                         "1 Scheduler, with the Submitter running as an "
                         "embedded thread inside rank 1).\n";
        }
        MPI_Finalize();
        return 1;
    }

    int port = 50051;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
    }

    // --------------------------------------------------------------
    // Rank 0: gRPC listener. See the shutdown-handling comment above for
    // the sigwait()-based shutdown sequence.
    // --------------------------------------------------------------
    if (rank == 0) {
        std::mutex mpi_mutex;
        MqssGrpcServer grpc_server = start_mqss_grpc_server(port, mpi_mutex);

        std::thread shutdown_watcher([&grpc_server, &shutdown_signals]() {
            int caught_signal = 0;
            printf("[P0] shutdown watcher: waiting in sigwait()\n");
            const int rc = sigwait(&shutdown_signals, &caught_signal);
            printf("[P0] shutdown watcher: sigwait() returned rc=%d "
                   "signal=%d, calling Shutdown()\n", rc, caught_signal);
            grpc_server.server->Shutdown();
            printf("[P0] shutdown watcher: Shutdown() returned\n");
        });

        grpc_server.server->Wait(); // returns once shutdown_watcher calls Shutdown()
        printf("[P0] server->Wait() returned\n");
        shutdown_watcher.join();
        printf("[P0] shutdown_watcher joined\n");

        // Propagate shutdown to rank 1 over the same MPI pipe real tasks
        // use - see the shutdown-handling comment above.
        mqss::scheduler::QuantumTask stop_task(kShutdownTaskId, /*priority=*/0);
        const std::string stop_msg = serialize_quantum_task(stop_task);
        {
            std::lock_guard<std::mutex> lock(mpi_mutex);
            MPI_Send(stop_msg.c_str(), static_cast<int>(stop_msg.size()) + 1,
                     MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        }
        printf("[P0] gRPC listener shut down, shutdown signal forwarded to "
               "rank 1\n");

    // --------------------------------------------------------------
    // Rank 1: Scheduler, with the QDMI Submitter running as an embedded
    // thread inside this same process rather than a further MPI rank -
    // see this file's top-of-file comment for why.
    // --------------------------------------------------------------
    } else if (rank == 1) {
        printf("[P%d] Initializing Scheduler\n", rank);
        mqss::scheduler::Scheduler<mqss::scheduler::QuantumTask> scheduler(mqss::scheduler::SchedulingPolicy::FirstInFirstOut);

        std::string driver_name = "qdmi_example_driver";
        std::string token = "test_token";
        auto session = std::make_unique<qdmi::Session>(driver_name, token);
        std::vector<qdmi::Device> devices = session->get_devices();
        if (devices.empty()) {
            std::cerr << "[P" << rank << "] No QDMI devices available.\n";
            MPI_Finalize();
            return 1;
        }
        qdmi::Device my_device = devices[0];
        Submitter submitter(my_device);
        printf("[P%d] Submitter thread: bound to device '%s'\n", rank,
               my_device.get_name().c_str());

        // Mirrors QRM workflow-aio/main.cpp's submitter_thread: poll
        // getNextReadyTask() and submit whatever comes back, sleeping
        // briefly when the queue is empty rather than busy-looping.
        std::atomic<bool> submitter_stop{false};
        std::thread submitter_thread([&]() {
            printf("[P%d] Submitter thread started. Waiting for ready "
                   "tasks to submit.\n", rank);
            while (!submitter_stop.load()) {
                auto next = scheduler.getNextReadyTask();
                if (!next.has_value()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                auto old_task = to_submitter_task(*next);
                printf("[P%d] Submitter thread: submitting task %d - %s\n", rank,
                       old_task->TaskId, old_task->CircuitFile.c_str());
                const QDMI_Job_Status status = submitter.submitTask(old_task, my_device);
                printf("[P%d] Submitter thread: task %d finished with status "
                       "%d, results written to %s\n",
                       rank, old_task->TaskId, static_cast<int>(status),
                       old_task->ResultsDestination.c_str());
            }
        });

        char buffer[65536]; // circuits can be larger than a 1024-byte buffer
        MPI_Status sched_status;
        while (true) {
            MPI_Recv(buffer, sizeof(buffer), MPI_CHAR, 0, 0, MPI_COMM_WORLD,
                     &sched_status);
            std::string msg(buffer);
            mqss::scheduler::QuantumTask task = deserialize_quantum_task(msg);

            if (task.task_id() == kShutdownTaskId) {
                printf("[P%d] Scheduler: shutdown signal received, "
                       "stopping submitter thread and finalizing\n", rank);
                submitter_stop.store(true);
                submitter_thread.join();
                break;
            }

            printf("[P%d] Scheduler: received task %d - %s, scheduling\n",
                   rank, task.task_id(),
                   task.circuit_files().empty() ? "" : task.circuit_files().front().c_str());
            scheduler.scheduleTask(task);
            printf("[P%d] Scheduler: task %d scheduled, %zu task(s) now "
                   "queued\n", rank, task.task_id(), scheduler.getTaskCount());
        }
    }

    MPI_Finalize();
    return 0;
}
