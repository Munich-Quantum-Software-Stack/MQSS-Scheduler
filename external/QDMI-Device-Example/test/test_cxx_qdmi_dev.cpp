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

#include "qdmi.hpp"
#include "qdmi/constants.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

class CxxQDMI_Test
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

// Initialize the test suite with input values
INSTANTIATE_TEST_SUITE_P(
    QDMITestInstantiation, CxxQDMI_Test,
    ::testing::Values(std::make_tuple("qdmi_example_driver", "test_token")));

// --------------------------------------------------------
// Test with parameters to check the QDMI session and device
// --------------------------------------------------------
TEST_P(CxxQDMI_Test, Session_GetDevices) {
    std::cout << "[TEST_P] Session_GetDevices: num_devices = "
            << session->get_devices().size() << std::endl;
    ASSERT_NE(session->get_devices().size(), 0);
}

TEST_P(CxxQDMI_Test, Session_GetDeviceName) {
    std::vector<qdmi::Device> devices = session->get_devices();
    for (const auto &device : devices) {
        std::cout << "[TEST_P] Session_GetDeviceName: "
                << device.get_name().c_str() << std::endl;
    }
    ASSERT_EQ(session->get_devices().size(), 1);
    for (const auto &device : devices) {
        std::string expected_name = "C++ Device with 5 qubits";
        ASSERT_STREQ(device.get_name().c_str(), expected_name.c_str());
    }
}

TEST_P(CxxQDMI_Test, Session_GetDeviceLibraryVersion) {
    std::vector<qdmi::Device> devices = session->get_devices();
    ASSERT_EQ(session->get_devices().size(), 1);
    for (const auto &device : devices) {
        std::string expected_lib_ver = "1.1.0";
        ASSERT_STREQ(device.get_library_version().c_str(), expected_lib_ver.c_str());
    }
}

TEST_P(CxxQDMI_Test, Session_GetDeviceNumQubits) {
    std::vector<qdmi::Device> devices = session->get_devices();
    ASSERT_EQ(session->get_devices().size(), 1);
    for (const auto &device : devices) {
        size_t expected_num_qubits = 5;
        std::cout << "[TEST_P] Session_GetDeviceNumQubits: "
                << device.get_num_qubits() << std::endl;
        ASSERT_EQ(device.get_num_qubits(), expected_num_qubits);
    }
}

TEST_P(CxxQDMI_Test, Session_GetDeviceOperations) {
    std::vector<qdmi::Device> devices = session->get_devices();
    ASSERT_EQ(session->get_devices().size(), 1);

    for (const auto &device : devices) {
        std::vector<std::pair<qdmi::Site, qdmi::Site>> coupling_map =
            device.get_coupling_map();
        std::vector<qdmi::Site> sites = device.get_sites();
        std::vector<qdmi::Operation> operations = device.get_operations();
        std::vector<std::tuple<std::string, size_t, size_t, double, double>>
        expected_operations = {
            std::make_tuple("rx", 1, 1, 0.01, 0.999),
            std::make_tuple("ry", 1, 1, 0.01, 0.999),
            std::make_tuple("rz", 1, 1, 0.01, 0.999),
            std::make_tuple("cx", 2, 0, 0.1, 0.99),
        };

        for (size_t i = 0; i < operations.size(); i++) {
            ASSERT_STREQ(operations[i].get_name().c_str(),
                    std::get<0>(expected_operations[i]).c_str());
            ASSERT_EQ(operations[i].get_num_qubits(),
                    std::get<1>(expected_operations[i]));
            ASSERT_EQ(operations[i].get_num_parameters(),
                    std::get<2>(expected_operations[i]));
            if (operations[i].get_num_qubits() == 1) {
                ASSERT_GT(sites.size(), 0);
                ASSERT_EQ(operations[i].get_duration({sites[0]}, {}),
                        std::get<3>(expected_operations[i]));
                ASSERT_EQ(operations[i].get_fidelity({sites[0]}),
                        std::get<4>(expected_operations[i]));
            } else if (operations[i].get_num_qubits() == 2) {
                ASSERT_GT(coupling_map.size(), 0);
                ASSERT_EQ(operations[i].get_duration(
                    {coupling_map[0].first, coupling_map[0].second}, {}),
                    std::get<3>(expected_operations[i]));
                ASSERT_EQ(operations[i].get_fidelity(
                    {coupling_map[0].first, coupling_map[0].second}),
                    std::get<4>(expected_operations[i]));
            } else {
                FAIL() << "Error: Cannot handel num. qubits "
                    << operations[i].get_num_qubits()
                    << " for operation: " << operations[i].get_name();
            }
        }
    }
}

TEST_P(CxxQDMI_Test, Session_CreateAndSubmitJob) {
    // get the num. of devices, should be one device for testing
    std::vector<qdmi::Device> devices = session->get_devices();
    ASSERT_EQ(session->get_devices().size(), 1);
    // for each device, create a tmp job and submit it
    for (const auto &device : devices) {
        qdmi::Job job = device.create_job(
            QDMI_Program_Format::QDMI_PROGRAM_FORMAT_QASM2,
            "qasm2 code",
            1000
        );
        ASSERT_EQ(job.status(), QDMI_Job_Status::QDMI_JOB_STATUS_CREATED);
        job.submit();
        job.wait();
        ASSERT_EQ(job.status(), QDMI_Job_Status::QDMI_JOB_STATUS_DONE);
    }
}

TEST_P(CxxQDMI_Test, Session_CancelJob) {
    // get the num. of devices, should be one device for testing
    std::vector<qdmi::Device> devices = session->get_devices();
    ASSERT_EQ(session->get_devices().size(), 1);
    // for each device, create a tmp job and submit it
    for (const auto &device : devices) {
        qdmi::Job job = device.create_job(
            QDMI_Program_Format::QDMI_PROGRAM_FORMAT_QASM2,
            "qasm2 code",
            1000
        );
        ASSERT_EQ(job.status(), QDMI_Job_Status::QDMI_JOB_STATUS_CREATED);
        job.cancel();
        ASSERT_EQ(job.status(), QDMI_Job_Status::QDMI_JOB_STATUS_CANCELED);
    }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}