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
#include <unistd.h>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>

#include <qinfo/qinfo.h>
#include <qdmi.hpp>
#include <qdmi/constants.h>

#include <submitter.hpp>
#include <quantum_task.hpp>

#include <scheduler/scheduler.hpp>
#include <scheduler/queue/queue.hpp>

int main(int argc, char **argv) {
    
    // Get the QDMI device info
    // Ensure the related dynamic libraries are loaded and QDMI config file is set
    std::string driver_name = "qdmi_example_driver";
    std::string token = "test_token";
    auto params = std::make_tuple(driver_name, token);
    std::unique_ptr<qdmi::Session> session;
    session = std::make_unique<qdmi::Session>(std::get<0>(params), std::get<1>(params));
    std::vector<qdmi::Device> devices = session->get_devices();
    for (const auto &device : devices) {
        std::cout << "CxxQDMISession_GetDeviceName: "
                  << device.get_name().c_str() << std::endl;
    }

    // Create a scheduler instance
    Scheduler mqss_scheduler;

    // --------------------------------------------------------------
    // Init the scheduler thread
    // --------------------------------------------------------------
    mqss_scheduler.initScheduler();
    
    // Create randomized circuits
    int num_qjobs = 5;
    for (int i = 0; i < num_qjobs; i++) {
        auto qtask = std::make_shared<QuantumTask>(i);
        qtask->NumberQbits = 2;
        qtask->NumberShots = 10000;
        qtask->CircuitFile = "qcircuit_"+std::to_string(i)+".qasm";
        qtask->CircuitFileType = "qasm3";
        qtask->ResultsDestination = "output_qcircuit_"+std::to_string(i)+".txt";
        mqss_scheduler.sched_queue->addJob(qtask);
    }
    std::cout << "[example]------------------------------------------" << std::endl;
    std::cout << "[example] Num. total jobs: " << mqss_scheduler.sched_queue->num_total_jobs << std::endl;
    std::cout << "[example]------------------------------------------" << std::endl;

    int job_id = 1;
    Submitter* my_qdmi_submitter = new Submitter(devices[0]);
    std::cout << "[example] Submit job id " << job_id << " to the submitter..." << std::endl;
    mqss_scheduler.handleJobs2Submitters(job_id, my_qdmi_submitter);

    // Push some sleep to check the scheduler thread
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // --------------------------------------------------------------
    // Finalize the scheduler thread
    // --------------------------------------------------------------
    mqss_scheduler.finiScheduler();

    return 0;
}   
