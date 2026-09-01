/*
 * Copyright (c) 2024 - 2026 Munich Quantum Software Stack
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

// Note: This project has a similar QuantumTask structure with `mqss::QuantumTask` 
// to keep consistent with the MQSS Quantum resource manager.

#ifndef MQSS_SCHEDULER_QUANTUM_TASK_HPP
#define MQSS_SCHEDULER_QUANTUM_TASK_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace mqss::scheduler {

/**
 * @brief Quantum Task follows the MQSS Quantum resource manager.
 * The task properties are based on the quantum scheduler concepts. 
 * In which, the task object is referred to protobuf to simplify 
 * the communication protocol, serialization, and deserialization.
 * 
 * Some attributes might be not required for the scheduling, but they 
 * might be useful for the task submission and execution.
 *  + via_hpc: whether the task is submitted via HPC or not.
 *  + circuit_files: the list of circuit files for the task.
 *  + circuit_file_type: the type of the circuit files, e.g., qasm
 *  + etc.
 */
class QuantumTask {
public:
    QuantumTask() = default;

    QuantumTask(int task_id, int priority, int n_qbits = 1, int n_shots = 1000)
        : task_id_(task_id), priority_(priority), n_qbits_(n_qbits), n_shots_(n_shots) {}

    // To keep the task properties matching with the predefined quantum task in 
    // the MQSS Quantum resource manager, the following variables are named 
    // according to the proto fields, mqss::protocol::v1::QuantumTask.
    // Private members below use a trailing underscore rather than being
    // bare `task_id` etc.
    int task_id() const { return task_id_; }
    int priority() const { return priority_; }
    int n_qbits() const { return n_qbits_; }
    int n_shots() const { return n_shots_; }
    const std::vector<std::string> &circuit_files() const { return circuit_files_; }
    const std::string &circuit_file_type() const { return circuit_file_type_; }
    const std::string &result_destination() const { return result_destination_; }
    const std::string &preferred_qpu() const { return preferred_qpu_; }
    const std::string &scheduled_qpu() const { return scheduled_qpu_; }

    // A standalone quantum task scheduler can have tasks arrived
    // either from an HPC job (co-located submission) or straight
    // from the remote cloud platform.
    bool via_hpc() const { return via_hpc_; }

    void set_task_id(int v) { task_id_ = v; }
    void set_priority(int v) { priority_ = v; }
    void set_n_qbits(int v) { n_qbits_ = v; }
    void set_n_shots(int v) { n_shots_ = v; }
    void add_circuit_file(std::string v) { circuit_files_.push_back(std::move(v)); }
    void set_circuit_file_type(std::string v) { circuit_file_type_ = std::move(v); }
    void set_result_destination(std::string v) { result_destination_ = std::move(v); }
    void set_preferred_qpu(std::string v) { preferred_qpu_ = std::move(v); }
    void set_scheduled_qpu(std::string v) { scheduled_qpu_ = std::move(v); }
    void set_via_hpc(bool v) { via_hpc_ = v; }

private:
    int task_id_ = 0;
    int priority_ = 0;
    int n_qbits_ = 1;
    int n_shots_ = 1000;
    std::vector<std::string> circuit_files_;
    std::string circuit_file_type_;
    std::string result_destination_;
    std::string preferred_qpu_;
    std::string scheduled_qpu_;
    bool via_hpc_ = false;
};

} // namespace mqss::scheduler

#endif // MQSS_SCHEDULER_QUANTUM_TASK_HPP
