/*------------------------------------------------------------------------------
Copyright 2024 - 2026 Munich Quantum Software Stack
All rights reserved.

Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://llvm.org/LICENSE.txt

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

#include "mqss_grpc_server.hpp"

#include <mpi.h>

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "quantum_job.grpc.pb.h"

// --------------------------------------------------------------
// Serialize/deserialize a mqss::QuantumTask as a string (for sending over
// MPI). quantum_job.proto's CircuitRequest carries only one circuit per
// task, so `circuit_files` is serialized/parsed as a single field here even
// though the type itself supports several (matching the real
// mqss.protocol.v1.QuantumTask's `repeated string circuit_files`).
// --------------------------------------------------------------
std::string serialize_quantum_task(const mqss::QuantumTask &task) {
    std::ostringstream oss;
    oss << task.task_id() << ";" << task.priority() << ";" << task.n_qbits() << ";"
        << task.n_shots() << ";"
        << (task.circuit_files().empty() ? "" : task.circuit_files().front()) << ";"
        << task.circuit_file_type() << ";" << task.result_destination() << ";"
        << (task.via_hpc() ? 1 : 0);
    return oss.str();
}

mqss::QuantumTask deserialize_quantum_task(const std::string &s) {
    std::istringstream iss(s);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(iss, token, ';')) parts.push_back(token);

    mqss::QuantumTask task(std::stoi(parts[0]), std::stoi(parts[1]), std::stoi(parts[2]),
                            std::stoi(parts[3]));
    if (!parts[4].empty()) {
        task.add_circuit_file(parts[4]);
    }
    task.set_circuit_file_type(parts[5]);
    task.set_result_destination(parts[6]);
    task.set_via_hpc(!parts[7].empty() && parts[7] != "0");

    return task;
}

namespace {

// Implements QuantumJobService from quantum_job.proto (the same contract
// mqss-scheduler-grpc/listener.py's Python stub speaks). Purely an
// implementation detail of start_mqss_grpc_server() below — nothing outside
// this file needs the class itself, only the running grpc::Server it gets
// registered on.
class QuantumJobServiceImpl final : public mqss::QuantumJobService::Service {
public:
    explicit QuantumJobServiceImpl(std::mutex &mpi_mutex) : mpi_mutex_(mpi_mutex) {
        std::filesystem::create_directories(circuits_dir_);
    }

    grpc::Status SubmitCircuit(grpc::ServerContext * /*context*/,
                               const mqss::CircuitRequest *request,
                               mqss::CircuitResult *response) override {
        const int task_id = next_task_id_.fetch_add(1);

        // QuantumTask exchanges circuits via the filesystem (circuit_files),
        // not inline data — write what arrived over gRPC to a local file.
        // Rank 0/1 (and rank 1's embedded submitter thread) share this
        // container's filesystem, so this needs no cross-container sharing,
        // unlike the gRPC hop itself.
        const std::string circuit_file = circuits_dir_ + "/job_" +
                                         std::to_string(task_id) + "." +
                                         request->program_format();
        std::ofstream ofs(circuit_file);
        ofs << request->program();
        ofs.close();

        // quantum_job.proto's CircuitRequest has no priority field, so
        // every task is scheduled at the same priority - Scheduler's
        // FirstInFirstOut policy (see qrm_sscheduler.cpp) makes this exact
        // FIFO-among-equals behavior explicit rather than a side effect of
        // an unused PriorityBased policy.
        mqss::QuantumTask task(task_id, /*priority=*/0);
        task.set_n_shots(request->shots());
        task.add_circuit_file(circuit_file);
        task.set_circuit_file_type(request->program_format());
        task.set_result_destination(circuits_dir_ + "/job_" + std::to_string(task_id) +
                                     "_result.txt");

        const std::string msg = serialize_quantum_task(task);
        {
            std::lock_guard<std::mutex> lock(mpi_mutex_);
            MPI_Send(msg.c_str(), static_cast<int>(msg.size()) + 1, MPI_CHAR, 1, 0,
                     MPI_COMM_WORLD);
        }

        printf("[P0] gRPC listener: accepted external job_id=%s qc_alias=%s "
               "shots=%d -> internal task_id=%d, forwarded to Scheduler\n",
               request->job_id().c_str(), request->qc_alias().c_str(),
               request->shots(), task_id);

        // Fire-and-forget: acknowledge acceptance into the pipeline, not
        // completion. `counts` is intentionally left empty.
        response->set_job_id(request->job_id());
        response->set_success(true);
        return grpc::Status::OK;
    }

private:
    std::mutex &mpi_mutex_;
    std::atomic<int> next_task_id_{0};
    const std::string circuits_dir_ = "/tmp/mqss-grpc-circuits";
};

} // namespace

MqssGrpcServer start_mqss_grpc_server(int port, std::mutex &mpi_mutex) {
    // Owned by the caller via the returned MqssGrpcServer, not this
    // function — ServerBuilder::RegisterService() only stores a raw
    // pointer to it, so it must outlive the server itself.
    auto service = std::make_unique<QuantumJobServiceImpl>(mpi_mutex);

    grpc::ServerBuilder builder;
    const std::string server_address = "[::]:" + std::to_string(port);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    printf("[P0] gRPC listener up on %s, forwarding accepted jobs to "
           "rank 1 (Scheduler)\n",
           server_address.c_str());
    return MqssGrpcServer{std::move(server), std::move(service)};
}
