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

#include "qdmi.hpp"
#include "qdmi/types.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

std::string qdmi::Operation::get_name() const {

    size_t size_ret{};
    callFuncAndThrowOnError(QDMI_device_query_operation_property_ptr,
                            "Failed to query operation name", device, operation, 0, nullptr, 0,
                            nullptr, QDMI_OPERATION_PROPERTY_NAME, 0, nullptr, &size_ret);
    std::vector<char> value(size_ret);
    callFuncAndThrowOnError(QDMI_device_query_operation_property_ptr,
                            "Failed to query operation name", device, operation, 0, nullptr, 0,
                            nullptr, QDMI_OPERATION_PROPERTY_NAME, size_ret, value.data(), nullptr);
    return std::string(value.data());
}

size_t qdmi::Operation::get_num_qubits() const {
    size_t num_qubits{};
    callFuncAndThrowOnError(QDMI_device_query_operation_property_ptr,
                            "Failed to query operation number of qubits", device, operation, 0,
                            nullptr, 0, nullptr, QDMI_OPERATION_PROPERTY_QUBITSNUM, sizeof(size_t),
                            &num_qubits, nullptr);
    return num_qubits;
}

size_t qdmi::Operation::get_num_parameters() const {
    size_t num_params{};
    callFuncAndThrowOnError(QDMI_device_query_operation_property_ptr,
                            "Failed to query operation number of parameters", device, operation, 0,
                            nullptr, 0, nullptr, QDMI_OPERATION_PROPERTY_PARAMETERSNUM,
                            sizeof(size_t), &num_params, nullptr);
    return num_params;
}

double qdmi::Operation::get_duration(const std::vector<std::reference_wrapper<Site>> &sites,
                                     const std::vector<double> &parameters) const {
    std::vector<QDMI_Site> sites_v;
    for (const auto &site : sites) {
        if (site.get().site != nullptr) {
            sites_v.push_back(site.get().site);
        }
    }
    double duration{};
    callFuncAndThrowOnError(QDMI_device_query_operation_property_ptr,
                            "Failed to query operation duration", device, operation, sites_v.size(),
                            sites_v.size() ? sites_v.data() : nullptr, parameters.size(),
                            parameters.size() ? parameters.data() : nullptr,
                            QDMI_OPERATION_PROPERTY_DURATION, sizeof(double), &duration, nullptr);
    return duration;
}

double qdmi::Operation::get_fidelity(const std::vector<std::reference_wrapper<Site>> &sites,
                                     const std::vector<double> &parameters) const {
    std::vector<QDMI_Site> sites_v;
    for (const auto &site : sites) {
        if (site.get().site != nullptr) {
            sites_v.push_back(site.get().site);
        }
    }
    double fidelity{};
    callFuncAndThrowOnError(QDMI_device_query_operation_property_ptr,
                            "Failed to query operation fidelity", device, operation, sites_v.size(),
                            sites_v.size() ? sites_v.data() : nullptr, parameters.size(),
                            parameters.size() ? parameters.data() : nullptr,
                            QDMI_OPERATION_PROPERTY_FIDELITY, sizeof(double), &fidelity, nullptr);
    return fidelity;
}
