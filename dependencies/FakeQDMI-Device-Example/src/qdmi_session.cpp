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

#include <cassert>
#include <iostream>

qdmi::Session::Session(const std::string &driver_name, const std::string &token,
                       const std::string &username, const std::string &projectid)
    : session(nullptr), qdmi_driver_lib(driver_name) {

    callFuncAndThrowOnError(QDMI_session_alloc_ptr, "Failed to allocate QDMI session", &session);

    set_token(token);
    set_username(username);
    set_projectid(projectid);

    callFuncAndThrowOnError(QDMI_session_init_ptr, "Failed to initialize QDMI session", session);
}

qdmi::Session::~Session() {
    if (session != nullptr) {
        QDMI_session_free_ptr(session);
        session = nullptr;
    }
    qdmi_devices.clear();
}

void qdmi::Session::set_token(const std::string &token_) {
    token = token_;
    if (token.empty()) {
        return;
    }
    callFuncAndThrowOnError(QDMI_session_set_parameter_ptr, "Failed to set token for the session",
                            session, QDMI_SESSION_PARAMETER_TOKEN, token.length() + 1,
                            token.c_str());
}

void qdmi::Session::set_username(const std::string &username_) {
    username = username_;
    if (username.empty()) {
        return;
    }
    callFuncAndThrowOnError(
        QDMI_session_set_parameter_ptr, "Failed to set username for the session", session,
        QDMI_SESSION_PARAMETER_USERNAME, username.length() + 1, username.c_str());
}

void qdmi::Session::set_projectid(const std::string &projectid_) {
    projectid = projectid_;
    if (projectid.empty()) {
        return;
    }
    callFuncAndThrowOnError(
        QDMI_session_set_parameter_ptr, "Failed to set project id for the session", session,
        QDMI_SESSION_PARAMETER_PROJECTID, projectid.length() + 1, projectid.c_str());
}

std::vector<qdmi::Device> qdmi::Session::get_devices() {
    if (session == nullptr) {
        throw std::runtime_error("Session is not initialized");
    }

    if (!qdmi_devices.empty()) {
        std::vector<qdmi::Device> device_list;
        device_list.reserve(qdmi_devices.size());
        for (const auto &device : qdmi_devices) {
            device_list.emplace_back(device);
        }
        return device_list;
    }

    size_t num_devices = 0;
    callFuncAndThrowOnError(QDMI_session_query_session_property_ptr,
                            "Failed to get number of devices", session,
                            QDMI_SESSION_PROPERTY_DEVICES, 0, nullptr, &num_devices);

    if (num_devices == 0) {
        return {};
    }

    qdmi_devices.resize(num_devices / sizeof(QDMI_Device));
    callFuncAndThrowOnError(QDMI_session_query_session_property_ptr, "Failed to get devices",
                            session, QDMI_SESSION_PROPERTY_DEVICES, num_devices,
                            qdmi_devices.data(), nullptr);

    std::vector<qdmi::Device> device_list;
    device_list.reserve(qdmi_devices.size());
    for (const auto &device : qdmi_devices) {
        device_list.emplace_back(device);
    }

    return device_list;
}
