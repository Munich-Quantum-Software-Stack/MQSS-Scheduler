/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

#include <unistd.h>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>
#include <gtest/gtest.h>

#include <qinfo/qinfo.h>
#include <qdmi.hpp>
#include <qdmi/constants.h>

#include <submitter.hpp>
#include <quantum_task.hpp>

#include "scheduler/scheduler.hpp"
#include "scheduler/queue/queue.hpp"


class Scheduler_QDMI_Test
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {

protected:
    void SetUp() override {
        auto params = GetParam();
        session = std::make_unique<qdmi::Session>(std::get<0>(params),
                                              std::get<1>(params));
    }

    void TearDown() override {}

    std::unique_ptr<qdmi::Session> session;
};

// --------------------------------------------------------
// Initialize the test suite with input values
// --------------------------------------------------------
INSTANTIATE_TEST_SUITE_P(
    MQSSSchedulerTestInstantiation, Scheduler_QDMI_Test,
    ::testing::Values(std::make_tuple("qdmi_example_driver", "test_token")));

// --------------------------------------------------------
// Test with parameters to check the QDMI session and device
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, CxxQDMISession_GetDevices) {
    std::cout << "[ TEST_P---] CxxQDMISession_GetDevices: num_devices = "
            << session->get_devices().size() << std::endl;
    ASSERT_NE(session->get_devices().size(), 0);
}

// --------------------------------------------------------
// Test QDMI Session getting device name
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, CxxQDMISession_GetDeviceName) {
    std::vector<qdmi::Device> devices = session->get_devices();
    for (const auto &device : devices) {
        std::cout << "[ TEST_P---] CxxQDMISession_GetDeviceName: "
                << device.get_name().c_str() << std::endl;
    }
    ASSERT_EQ(session->get_devices().size(), 1);
    for (const auto &device : devices) {
        std::string expected_name = "C++ Device with 5 qubits";
        ASSERT_STREQ(device.get_name().c_str(), expected_name.c_str());
    }
}

// --------------------------------------------------------
// Test Scheduler Queue Constructor and Destructor
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, SchedulerQueue_ConstructorDestructor) {
    std::cout << "[ TEST_P---] SchedulerQueue_ConstructorDestructor"
              << std::endl;
    SchedulerQueue sched_queue;
    EXPECT_NE(&sched_queue, nullptr);
}

// --------------------------------------------------------
// Test Scheduler Queue adding jobs
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, SchedulerQueue_AddJob) {
    std::cout << "[ TEST_P---] SchedulerQueue_AddJob"
              << std::endl;
    // create a scheduler queue
    SchedulerQueue sched_queue;    
    // create quantum jobs
    int num_qjobs = 5;
    for (int i = 0; i < num_qjobs; i++) {
        auto qtask = std::make_shared<QuantumTask>(i);
        qtask->NumberQbits = 2;
        qtask->NumberShots = 10000;
        qtask->CircuitFile = "qcircuit_"+std::to_string(i)+".qasm";
        qtask->CircuitFileType = "qasm3";
        qtask->ResultsDestination = "output_qcircuit_"+std::to_string(i)+".txt";
        std::cout << "Task ID " << qtask->TaskId << std::endl;
        // add jobs to the queue
        sched_queue.addJob(qtask);
    }
    std::cout << "Num. total jobs: " << sched_queue.num_total_jobs << std::endl;

    // check the number of jobs in the queue
    int num_total_jobs = sched_queue.jobs.size();
    ASSERT_EQ(num_total_jobs, 5);
    ASSERT_EQ(sched_queue.num_total_jobs, 5);
}

// --------------------------------------------------------
// Test Scheduler Queue removing jobs
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, SchedulerQueue_RemoveJob) {
    std::cout << "[ TEST_P---] SchedulerQueue_RemoveJob" << std::endl;
    // create a scheduler queue
    SchedulerQueue sched_queue;
    // create quantum jobs
    int num_qjobs = 5;
    for (int i = 0; i < num_qjobs; i++) {
        auto qtask = std::make_shared<QuantumTask>(i);
        qtask->NumberQbits = 2;
        qtask->NumberShots = 10000;
        qtask->CircuitFile = "qcircuit_"+std::to_string(i)+".qasm";
        qtask->CircuitFileType = "qasm3";
        qtask->ResultsDestination = "output_qcircuit_"+std::to_string(i)+".txt";
        std::cout << "Task ID " << qtask->TaskId << std::endl;
        // add jobs to the queue
        sched_queue.addJob(qtask);
    }
    // assume remove the first job
    auto qtask_to_remove = sched_queue.jobs.front();
    std::cout << "Removing Task ID " << qtask_to_remove->TaskId << std::endl;
    sched_queue.removeJob(qtask_to_remove);

    // check the number of jobs in the queue
    int num_total_jobs = sched_queue.jobs.size();
    ASSERT_EQ(num_total_jobs, 4);
    ASSERT_EQ(sched_queue.num_total_jobs, 4);
}

// --------------------------------------------------------
// Test Scheduler Queue getting jobs
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, SchedulerQueue_GetJob) {
    std::cout << "[ TEST_P---] SchedulerQueue_GetJob" << std::endl;
    // create a scheduler queue
    SchedulerQueue sched_queue;
    // create quantum jobs
    int num_qjobs = 5;
    for (int i = 0; i < num_qjobs; i++) {
        auto qtask = std::make_shared<QuantumTask>(i);
        qtask->NumberQbits = 2;
        qtask->NumberShots = 10000;
        qtask->CircuitFile = "qcircuit_"+std::to_string(i)+".qasm";
        qtask->CircuitFileType = "qasm3";
        qtask->ResultsDestination = "output_qcircuit_"+std::to_string(i)+".txt";
        std::cout << "Task ID " << qtask->TaskId << std::endl;
        // add jobs to the queue
        sched_queue.addJob(qtask);
    }
    // assume remove the first job
    std::shared_ptr<QuantumTask> selected_qtask;
    sched_queue.getJob(0, selected_qtask);
    std::cout << "Getting Job ID " << selected_qtask->TaskId << std::endl;

    // check the number of jobs in the queue
    int num_total_jobs = sched_queue.jobs.size();
    std::cout << "Remaining Jobs: " << num_total_jobs << std::endl;
    ASSERT_EQ(sched_queue.num_total_jobs, 4);
}

// --------------------------------------------------------
// Test Submitter initialization and finalization
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, Scheduler_InitFini) {
    Scheduler mqss_scheduler;
    mqss_scheduler.initScheduler();
    std::cout << "[ TEST_P---] Init/Fini Scheduler Thread" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    mqss_scheduler.finiScheduler();
    EXPECT_NE(&mqss_scheduler, nullptr);
}

// --------------------------------------------------------
// Test Submitter initialization and finalization
// --------------------------------------------------------
TEST_P(Scheduler_QDMI_Test, Scheduler_HandleJobs2Submitters) {
    Scheduler mqss_scheduler;
    SchedulerQueue sched_queue;
    int num_qjobs = 5;
    for (int i = 0; i < num_qjobs; i++) {
        auto qtask = std::make_shared<QuantumTask>(i);
        qtask->NumberQbits = 2;
        qtask->NumberShots = 10000;
        qtask->CircuitFile = "qcircuit_"+std::to_string(i)+".qasm";
        qtask->CircuitFileType = "qasm3";
        qtask->ResultsDestination = "output_qcircuit_"+std::to_string(i)+".txt";
        sched_queue.addJob(qtask);
    }
    
    // assign the scheduler queue to the queue object
    mqss_scheduler.sched_queue = std::make_shared<SchedulerQueue>(sched_queue);
    // assume schedule the 2nd first job to the submitter
    int job_id = 1;
    qdmi::Device device = session->get_devices()[0];
    Submitter* my_qdmi_submitter = new Submitter(device);
    mqss_scheduler.handleJobs2Submitters(job_id, my_qdmi_submitter);
    std::cout << "[ TEST_P---] Passed handleJobs2Sumitters" << std::endl;
    
    int num_jobs_in_submitter = my_qdmi_submitter->getQueueSize();
    EXPECT_EQ(num_jobs_in_submitter, 1);
}
