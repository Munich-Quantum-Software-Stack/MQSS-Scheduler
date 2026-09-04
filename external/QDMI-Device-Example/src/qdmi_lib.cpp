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

#include "qdmi_lib.hpp"

#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>

QDMI_driver_init_t QDMI_driver_init_ptr{nullptr};
QDMI_driver_shutdown_t QDMI_driver_shutdown_ptr{nullptr};
QDMI_session_alloc_t QDMI_session_alloc_ptr{nullptr};
QDMI_session_set_parameter_t QDMI_session_set_parameter_ptr{nullptr};
QDMI_session_init_t QDMI_session_init_ptr{nullptr};
QDMI_session_query_session_property_t QDMI_session_query_session_property_ptr{nullptr};
QDMI_session_free_t QDMI_session_free_ptr{nullptr};
QDMI_device_query_device_property_t QDMI_device_query_device_property_ptr{nullptr};
QDMI_device_query_site_property_t QDMI_device_query_site_property_ptr{nullptr};
QDMI_device_query_operation_property_t QDMI_device_query_operation_property_ptr{nullptr};
QDMI_device_create_job_t QDMI_device_create_job_ptr{nullptr};
QDMI_job_set_parameter_t QDMI_job_set_parameter_ptr{nullptr};
QDMI_job_submit_t QDMI_job_submit_ptr{nullptr};
QDMI_job_cancel_t QDMI_job_cancel_ptr{nullptr};
QDMI_job_check_t QDMI_job_check_ptr{nullptr};
QDMI_job_wait_t QDMI_job_wait_ptr{nullptr};
QDMI_job_get_results_t QDMI_job_get_results_ptr{nullptr};
QDMI_job_free_t QDMI_job_free_ptr{nullptr};

QDMIDriverLibrary::QDMIDriverLibrary(const std::string &library_name) {

    // if library_name is empty, check the env var QDMI_DRIVER_LIB_NAME if it is
    // also not available then use the default library name
    std::string lib_name =
        library_name.empty()
            ? (getenv("QDMI_DRIVER_LIB_NAME") ? getenv("QDMI_DRIVER_LIB_NAME") : "qdmi_driver")
            : library_name;

#ifdef __linux__
    std::string qdmi_driver_lib_name = "lib" + lib_name + ".so";
#elif __APPLE__
    std::string qdmi_driver_lib_name = "lib" + lib_name + ".dylib";
#endif

    std::string env_path = getenv("LD_LIBRARY_PATH") ? getenv("LD_LIBRARY_PATH") : "";
    std::vector<std::string> search_paths = splitString(env_path, ":");
    for (const auto &path : {"/usr/local/lib/", "/usr/lib/", "/lib/"}) {
        search_paths.emplace_back(path);
    }

    bool lib_found = false;
    for (const auto &path : search_paths) {
        if (std::filesystem::exists(path + "/" + qdmi_driver_lib_name)) {
            qdmi_driver_lib_name = path + "/" + qdmi_driver_lib_name;
            lib_found = true;
            break;
        }
    }

    if (!lib_found) {
        throw std::runtime_error("QDMI Driver library (" + qdmi_driver_lib_name + ") not found");
    }

    qdmi_driver_lib_handle.reset(dlopen(qdmi_driver_lib_name.c_str(), RTLD_LAZY));
    if (!qdmi_driver_lib_handle) {
        throw std::runtime_error("Failed to load QDMI Driver library: " + std::string(dlerror()));
    }

    load_symbol(QDMI_driver_init_ptr, "QDMI_driver_init");
    load_symbol(QDMI_driver_shutdown_ptr, "QDMI_driver_shutdown");
    load_symbol(QDMI_session_alloc_ptr, "QDMI_session_alloc");
    load_symbol(QDMI_session_set_parameter_ptr, "QDMI_session_set_parameter");
    load_symbol(QDMI_session_init_ptr, "QDMI_session_init");
    load_symbol(QDMI_session_query_session_property_ptr, "QDMI_session_query_session_property");
    load_symbol(QDMI_session_free_ptr, "QDMI_session_free");
    load_symbol(QDMI_device_query_device_property_ptr, "QDMI_device_query_device_property");
    load_symbol(QDMI_device_query_site_property_ptr, "QDMI_device_query_site_property");
    load_symbol(QDMI_device_query_operation_property_ptr, "QDMI_device_query_operation_property");
    load_symbol(QDMI_device_create_job_ptr, "QDMI_device_create_job");
    load_symbol(QDMI_job_set_parameter_ptr, "QDMI_job_set_parameter");
    load_symbol(QDMI_job_submit_ptr, "QDMI_job_submit");
    load_symbol(QDMI_job_cancel_ptr, "QDMI_job_cancel");
    load_symbol(QDMI_job_check_ptr, "QDMI_job_check");
    load_symbol(QDMI_job_wait_ptr, "QDMI_job_wait");
    load_symbol(QDMI_job_get_results_ptr, "QDMI_job_get_results");
    load_symbol(QDMI_job_free_ptr, "QDMI_job_free");
    int status_code = QDMI_driver_init_ptr();
    if (status_code != QDMI_SUCCESS) {
        throw std::runtime_error("Failed to initialize QDMI Driver library: " +
                                 std::to_string(status_code));
    }
}
QDMIDriverLibrary::~QDMIDriverLibrary() {
    if (QDMI_driver_shutdown_ptr != nullptr) {
        int status_code = QDMI_driver_shutdown_ptr();
        if (status_code != QDMI_SUCCESS) {
            std::cout << "Failed to shutdown QDMI Driver library: " << std::to_string(status_code)
                      << '\n';
        }
    }
}

std::vector<std::string> QDMIDriverLibrary::splitString(const std::string &str,
                                                        const std::string &delim) {
    std::regex regex(delim);
    std::sregex_token_iterator it(str.begin(), str.end(), regex, -1);
    std::sregex_token_iterator end;

    return {it, end};
}
