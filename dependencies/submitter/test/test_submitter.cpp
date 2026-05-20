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
#include <qinfo/qinfo.h>
#include <qdmi.hpp>
#include <qdmi/constants.h>

#include "submitter.hpp"
#include "quantum_task.hpp"

#include <filesystem>
#include <unistd.h>
#include <cstddef>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

class Submitter_QDMI_Test
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
    CxxQDMITestInstantiation, Submitter_QDMI_Test,
    ::testing::Values(std::make_tuple("qdmi_example_driver", "test_token")));

// --------------------------------------------------------
// Test with parameters to check the QDMI session and device
// --------------------------------------------------------
TEST_P(Submitter_QDMI_Test, CxxQDMISession_GetDevices) {
    std::cout << "[ TEST_P---] CxxQDMISession_GetDevices: num_devices = "
            << session->get_devices().size() << std::endl;
    ASSERT_NE(session->get_devices().size(), 0);
}

// --------------------------------------------------------
// Test QDMI Session getting device name
// --------------------------------------------------------
TEST_P(Submitter_QDMI_Test, CxxQDMISession_GetDeviceName) {
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
// Test Submitter constructors
// --------------------------------------------------------
TEST_P(Submitter_QDMI_Test, Submitter_Constructor1) {
  Submitter qdmi_submitter1;
  std::cout << "[ TEST_P---] Submitter Constructor()" << std::endl;
  EXPECT_NE(&qdmi_submitter1, nullptr);
}

TEST_P(Submitter_QDMI_Test, Submitter_Constructor2) {
  qdmi::Device device = session->get_devices()[0];
  Submitter qdmi_submitter2(device);
  std::cout << "[ TEST_P---] Submitter Constructor(device): "
            << device.get_name().c_str() << std::endl;
  EXPECT_NE(&qdmi_submitter2, nullptr);
}

// --------------------------------------------------------
// Test Submitter initialization and finalization
// --------------------------------------------------------
TEST_P(Submitter_QDMI_Test, Submitter_InitFini) {
  qdmi::Device device = session->get_devices()[0];
  Submitter qdmi_submitter;
  qdmi_submitter.initSubmitter(device);
  std::cout << "[ TEST_P---] Init/Fini Submitter Thread" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  qdmi_submitter.finiSubmitter(device);
  EXPECT_NE(&qdmi_submitter, nullptr);
}

// --------------------------------------------------------
// Test enqueuing tasks to the queue of a Submitter
// --------------------------------------------------------
TEST_P(Submitter_QDMI_Test, Submitter_Enqueue) {
  qdmi::Device device = session->get_devices()[0];
  Submitter qdmi_submitter;
  // Create quantum tasks
  std::cout << "[ TEST_P---] Enqueued Tasks to Submitter Queue:" << std::endl;
  int num_tasks = 5;
  for (int i = 0; i < num_tasks; i++) {
    auto task = std::make_shared<QuantumTask>(i);
    task->NumberQbits = 2;
    task->NumberShots = 10000;
    task->CircuitFile = "qcircuit_"+std::to_string(i)+".qasm";
    task->CircuitFileType = "qasm3";
    task->ResultsDestination = "output_qcircuit_"+std::to_string(i)+".txt";
    qdmi_submitter.enqueue(task);
    std::cout << "Task ID " << task->TaskId << std::endl;
  }
  ASSERT_EQ(qdmi_submitter.getQueueSize(), 5);
}

// --------------------------------------------------------
// Test sudmitting tasks to the QDMI device
// --------------------------------------------------------
TEST_P(Submitter_QDMI_Test, Submitter_TaskSubmission) {
  qdmi::Device device = session->get_devices()[0];
  Submitter qdmi_submitter;
  // Create quantum tasks
  int task_id = 101;
  auto task = std::make_shared<QuantumTask>(task_id);
  task->NumberQbits = 2;
  task->NumberShots = 10000;
  task->CircuitFile = "qcircuit_"+std::to_string(task_id)+".qasm";
  task->CircuitFileType = "qasm2";
  task->ResultsDestination = "output_qcircuit_"+std::to_string(task_id)+".txt";
  // Submit the task
  std::cout << "[ TEST_P---] Submit a task to the QDMI device" << std::endl;
  QDMI_Job_Status status;
  status = qdmi_submitter.submitTask(task, device);
  ASSERT_EQ(status, QDMI_Job_Status::QDMI_JOB_STATUS_DONE);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// enum class TEST_SESSION_MODE : uint8_t { READONLY, READWRITE };

// int main() {
//   std::cout << "[Submitter] Starting a submitter." << std::endl;

//   // Need Qinfo to setup the device info
//   QDMI_Session session = nullptr;
//   int err_session_init = QDMI_session_init(session);
//   CHECK_ERR(err_session_init, "QDMI_session_init");

//   // Assume there are two devices
//   int count = 2;

//   // Create the device and sumitter objects
//   std::vector<std::unique_ptr<Submitter>> submitters;
//   std::vector<QDMI_Device> devices = {};

//   // Add one submitter for each device
//   for (int index = 0; index < count; index++) {
//     QDMI_Device dev = nullptr;
    
//     int err_device_init = QDMI_device_initialize();
//     CHECK_ERR(err_device_init, "QDMI_core_open_device");

//     devices.push_back(dev);

//     submitters.push_back(std::make_unique<Submitter>(dev, 2));
//   }

//   // -------------------------------------------------------
//   // Try to stop the workflow
//   // -------------------------------------------------------
//   // std::exit(1);

//   std::cout << "[Submitter] Created a submitter for each device." << std::endl;
//   for (auto &dev : devices) {
//     QDMI_Device_property prop = BACKEND_NAME;
//     char* name;
//     err = QDMI_device_query_device_property(device, prop, &name);
//     CHECK_ERR(err, "QDMI_query_device_property_c");
//     std::cout << "   [Test]................Device " << name << std::endl;
//   }
//   // Load example circuit
//   std::string inputFile = "bell_state.ll";
//   std::cout << "   [Test]................Loading example circuit: " << inputFile
//             << std::endl;
//   std::filesystem::path currentFilePath = __FILE__; // Get the current file path
//   std::filesystem::path currentDir =
//       currentFilePath.parent_path(); // Get the directory of the current file
//   std::filesystem::path inputFilePath =
//       currentDir / "inputs" / inputFile; // Construct the relative path

//   // Distrubute random tasks among the submitters
//   int numOfTasks = 10 * submitters.size();
//   std::cout << "   [Test]................There will be " << numOfTasks
//             << " tasks created." << std::endl;

//   // Simulate the receival of tasks after optimization
//   int id = 0;
//   while (id < numOfTasks) {

//     // Generate a random value < 1k
//     int randomValue = std::rand() % 1000;

//     // Create a new quantum task
//     auto mQuantumTask = std::make_shared<QuantumTask>(id);

//     // Create a new context and module for each task
//     SMDiagnostic error;
//     orc::ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
//     std::unique_ptr<Module> module =
//         parseIRFile(inputFilePath.string(), error, *(TSCtx.getContext()));
//     ThreadSafeModule TSM =
//         ThreadSafeModule(std::move(module), std::move(TSCtx));

//     mQuantumTask->mThreadSafeModule = std::move(TSM);
//     mQuantumTask->mExecutionOrder = static_cast<double>(randomValue);

//     std::cout << "   [Test]................Created task with ID "
//               << mQuantumTask->mTaskId << " and execution order "
//               << mQuantumTask->mExecutionOrder << std::endl;

//     // Assign the task to a random submitter
//     int submitter_idx = randomValue % submitters.size();
//     submitters[submitter_idx]->enqueue(mQuantumTask);

//     QDMI_Device_property prop = BACKEND_NAME;
//     char* name;
//     int result = QDMI_query_device_property_c(
//         submitters[submitter_idx]->mDevice, prop, &name);
//     CHECK_ERR(err, "QDMI_query_device_property_c");
//     std::cout << "   [Test]................Task with ID "
//               << mQuantumTask->mTaskId << " assigned to Submitter for device "
//               << name << std::endl;

//     std::cout << "   [Test]................Currently "
//               << submitters[submitter_idx]->getQueueSize()
//               << " task(s) in the queue." << std::endl;
//     id++;
//   }

//   while (!std::all_of(submitters.begin(), submitters.end(), isQueueEmpty)) {
//   }
//   std::cout << "   [Test]................All tasks have been submitted."
//             << std::endl;

//   return 0;
// }