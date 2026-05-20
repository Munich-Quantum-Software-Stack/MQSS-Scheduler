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

#include <cstddef>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// Specialization for size_t
template <>
auto qdmi::Device::query_property<size_t>(QDMI_Device_Property property,
                                          const char *error_message) const {
    size_t value{};
    callFuncAndThrowOnError(QDMI_device_query_device_property_ptr, error_message, device, property,
                            sizeof(size_t), &value, nullptr);
    return value;
}

template <typename T>
auto qdmi::Device::query_property(QDMI_Device_Property property, const char *error_message) const {
    size_t size_ret{};
    callFuncAndThrowOnError(QDMI_device_query_device_property_ptr, error_message, device, property,
                            0, nullptr, &size_ret);

    std::vector<T> value(size_ret / sizeof(T));
    callFuncAndThrowOnError(QDMI_device_query_device_property_ptr, error_message, device, property,
                            size_ret, static_cast<void *>(value.data()), nullptr);
    return value;
}

std::string qdmi::Device::get_name() const {
    auto char_v = query_property<char>(QDMI_DEVICE_PROPERTY_NAME, "Failed to query device name");
    return std::string(char_v.data());
}

std::string qdmi::Device::get_version() const {
    auto char_v =
        query_property<char>(QDMI_DEVICE_PROPERTY_VERSION, "Failed to query device version");
    return std::string(char_v.data());
}

size_t qdmi::Device::get_status() const {
    return query_property<size_t>(QDMI_DEVICE_PROPERTY_STATUS, "Failed to query device status");
}

std::string qdmi::Device::get_library_version() const {
    auto char_v = query_property<char>(QDMI_DEVICE_PROPERTY_LIBRARYVERSION,
                                       "Failed to query device library version");
    return std::string{char_v.data()};
}

size_t qdmi::Device::get_num_qubits() const {
    return query_property<size_t>(QDMI_DEVICE_PROPERTY_QUBITSNUM,
                                  "Failed to query device num qubits");
}

std::vector<qdmi::Site> qdmi::Device::get_sites() const {
    auto sites_v =
        query_property<QDMI_Site>(QDMI_DEVICE_PROPERTY_SITES, "Failed to query device sites");
    std::vector<qdmi::Site> sites;
    sites.reserve(sites_v.size());
    for (auto site : sites_v) {
        sites.emplace_back(device, site);
    }
    return sites;
}

std::vector<qdmi::Operation> qdmi::Device::get_operations() const {
    auto operations_v = query_property<QDMI_Operation>(QDMI_DEVICE_PROPERTY_OPERATIONS,
                                                       "Failed to query device operations");
    std::vector<qdmi::Operation> operations;
    operations.reserve(operations_v.size());
    for (auto operation : operations_v) {
        operations.emplace_back(device, operation);
    }
    return operations;
}

std::vector<std::pair<qdmi::Site, qdmi::Site>> qdmi::Device::get_coupling_map() const {
    auto coupling_map_v = query_property<std::pair<QDMI_Site, QDMI_Site>>(
        QDMI_DEVICE_PROPERTY_COUPLINGMAP, "Failed to query device coupling map");
    std::vector<std::pair<qdmi::Site, qdmi::Site>> coupling_map;
    coupling_map.reserve(coupling_map_v.size());
    for (const auto &pair : coupling_map_v) {
        coupling_map.emplace_back(std::piecewise_construct,
                                  std::forward_as_tuple(device, pair.first),
                                  std::forward_as_tuple(device, pair.second));
    }
    return coupling_map;
}

bool qdmi::Device::needs_calibration() const {
    return static_cast<bool>(query_property<size_t>(QDMI_DEVICE_PROPERTY_NEEDSCALIBRATION,
                                                    "Failed to query device calibration status"));
}

qdmi::Job qdmi::Device::create_job(QDMI_Program_Format format, const std::string &prog,
                                   const size_t &num_shots) const {
    QDMI_Job job = nullptr;

    callFuncAndThrowOnError(QDMI_device_create_job_ptr, "Failed to create job", device, &job);

    callFuncAndThrowOnError(QDMI_job_set_parameter_ptr, "Failed to set program format", job,
                            QDMI_JOB_PARAMETER_PROGRAMFORMAT, sizeof(format), &format);

    callFuncAndThrowOnError(QDMI_job_set_parameter_ptr, "Failed to set program", job,
                            QDMI_JOB_PARAMETER_PROGRAM, prog.size(),
                            static_cast<const void *>(prog.data()));

    size_t n_shots = num_shots;
    if (n_shots > 0) {
        callFuncAndThrowOnError(QDMI_job_set_parameter_ptr, "Failed to set number of shots", job,
                                QDMI_JOB_PARAMETER_SHOTSNUM, sizeof(size_t), &n_shots);
    } else {
        throw std::runtime_error("Number of shots must be greater than 0");
    }

    return std::move(qdmi::Job{job});
}
