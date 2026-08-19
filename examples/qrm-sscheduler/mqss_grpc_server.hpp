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

// The gRPC-facing half of the pipeline: receives circuits from outside this
// container (see slurm-spank-plugins/mqss-spank-plugin/test/
// bell_state_offload.py's --provider mqss-grpc) and forwards them to MPI
// rank 1 as a mqss::QuantumTask (Scheduler's lightweight task type - see
// include/scheduler/quantum_task.hpp - not the older, LLVM/JIT-coupled
// global QuantumTask struct the Submitter still expects). Everything past
// that point (Scheduler -> embedded Submitter thread -> QDMI) lives in
// qrm_sscheduler.cpp, unmodified by which rank 0 implementation is in
// front of it.

#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <grpcpp/grpcpp.h>
// grpcpp.h alone only forward-declares grpc::Service — MqssGrpcServer below
// holds a std::unique_ptr<grpc::Service>, whose destructor needs the full
// definition wherever it's instantiated (i.e. anywhere a MqssGrpcServer
// goes out of scope, not just in mqss_grpc_server.cpp).
#include <grpcpp/impl/service_type.h>

#include <scheduler/quantum_task.hpp>

// Shutdown-sentinel task_id, used across both ranks to signal a clean
// shutdown instead of a real task — see qrm_sscheduler.cpp's
// shutdown-handling comment for how it's actually used end to end.
constexpr int kShutdownTaskId = -1;

// Wire format for a mqss::QuantumTask sent over MPI between rank 0 (this
// file) and rank 1 (qrm_sscheduler.cpp) — carries Scheduler's task type
// and its snake_case field names, distinct from the older struct's wire
// format.
std::string serialize_quantum_task(const mqss::QuantumTask &task);
mqss::QuantumTask deserialize_quantum_task(const std::string &s);

// A running QuantumJobService gRPC listener, plus the grpc::Service it's
// registered with — grpc::ServerBuilder::RegisterService() only stores a
// raw pointer to the service, it does not take ownership, so the two must
// be kept alive together for as long as the server runs. Destroying this
// struct is enough to tear down both in the right order (server first).
struct MqssGrpcServer {
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<grpc::Service> service;
};

// Builds and starts the QuantumJobService gRPC listener on `port`. Every
// accepted SubmitCircuit call is forwarded to MPI rank 1 via MPI_Send,
// guarded by `mpi_mutex` (required since MPI was only initialized with
// MPI_THREAD_SERIALIZED — gRPC's sync server dispatches concurrent RPCs
// onto its own thread pool, and that guarantee only holds if calls into
// MPI never overlap). `mpi_mutex` must outlive the returned server.
//
// To stop it: call .server->Shutdown() (safe to do from a thread other
// than the one calling .server->Wait() — see qrm_sscheduler.cpp's
// shutdown-handling comment for why that distinction matters), then
// .server->Wait() on the thread that should block until it's actually down.
MqssGrpcServer start_mqss_grpc_server(int port, std::mutex &mpi_mutex);
