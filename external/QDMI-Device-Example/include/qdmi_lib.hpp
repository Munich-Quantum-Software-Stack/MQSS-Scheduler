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
#pragma once

#include "qdmi/client.h"

#include <dlfcn.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Function pointers type declarations for the QDMI client functions
using QDMI_driver_init_t = int (*)();
using QDMI_driver_shutdown_t = int (*)();
using QDMI_session_alloc_t = int (*)(QDMI_Session *);
using QDMI_session_set_parameter_t = int (*)(QDMI_Session, QDMI_Session_Parameter, size_t,
                                             const void *);
using QDMI_session_init_t = int (*)(QDMI_Session);
using QDMI_session_query_session_property_t = int (*)(QDMI_Session, QDMI_Session_Property, size_t,
                                                      void *, size_t *);
using QDMI_session_free_t = void (*)(QDMI_Session);
using QDMI_device_query_device_property_t = int (*)(QDMI_Device, QDMI_Device_Property, size_t,
                                                    void *, size_t *);
using QDMI_device_query_site_property_t = int (*)(QDMI_Device, QDMI_Site, QDMI_Site_Property,
                                                  size_t, void *, size_t *);
using QDMI_device_query_operation_property_t = int (*)(QDMI_Device, QDMI_Operation, const size_t,
                                                       const QDMI_Site *, size_t, const double *,
                                                       QDMI_Operation_Property, size_t, void *,
                                                       size_t *);
using QDMI_device_create_job_t = int (*)(QDMI_Device, QDMI_Job *);
using QDMI_job_set_parameter_t = int (*)(QDMI_Job, QDMI_Job_Parameter, size_t, const void *);
using QDMI_job_submit_t = int (*)(QDMI_Job);
using QDMI_job_cancel_t = int (*)(QDMI_Job);
using QDMI_job_check_t = int (*)(QDMI_Job, QDMI_Job_Status *);
// QDMI_job_wait gained a `size_t timeout` (seconds; 0 = wait indefinitely,
// preserving the pre-v1.2.x behavior) parameter as of QDMI v1.2.x. Since
// this pointer is resolved via dlsym (not statically linked against
// client.h's declaration), a stale 1-arg signature here would compile fine
// but silently miscall the real 2-arg function at runtime — this must be
// kept in sync with whatever QDMI version external/QDMI is pinned to.
using QDMI_job_wait_t = int (*)(QDMI_Job, size_t);
using QDMI_job_get_results_t = int (*)(QDMI_Job, QDMI_Job_Result, size_t, void *, size_t *);
using QDMI_job_free_t = void (*)(QDMI_Job);

// Function pointers to the QDMI client functions
extern QDMI_driver_init_t QDMI_driver_init_ptr;
extern QDMI_driver_shutdown_t QDMI_driver_shutdown_ptr;
extern QDMI_session_alloc_t QDMI_session_alloc_ptr;
extern QDMI_session_set_parameter_t QDMI_session_set_parameter_ptr;
extern QDMI_session_init_t QDMI_session_init_ptr;
extern QDMI_session_query_session_property_t QDMI_session_query_session_property_ptr;
extern QDMI_session_free_t QDMI_session_free_ptr;
extern QDMI_device_query_device_property_t QDMI_device_query_device_property_ptr;
extern QDMI_device_query_site_property_t QDMI_device_query_site_property_ptr;
extern QDMI_device_query_operation_property_t QDMI_device_query_operation_property_ptr;
extern QDMI_device_create_job_t QDMI_device_create_job_ptr;
extern QDMI_job_set_parameter_t QDMI_job_set_parameter_ptr;
extern QDMI_job_submit_t QDMI_job_submit_ptr;
extern QDMI_job_cancel_t QDMI_job_cancel_ptr;
extern QDMI_job_check_t QDMI_job_check_ptr;
extern QDMI_job_wait_t QDMI_job_wait_ptr;
extern QDMI_job_get_results_t QDMI_job_get_results_ptr;
extern QDMI_job_free_t QDMI_job_free_ptr;

// Class to manage the QDMI driver library
class QDMIDriverLibrary {
public:
    QDMIDriverLibrary(const std::string &library_name = "");
    ~QDMIDriverLibrary();

private:
    std::unique_ptr<void, int (*)(void *)> qdmi_driver_lib_handle{nullptr, &dlclose};

    template <typename T> void load_symbol(T &func_ptr, const char *symbol_name) {
        func_ptr = reinterpret_cast<T>(dlsym(qdmi_driver_lib_handle.get(), symbol_name));
        if (!func_ptr) {
            throw std::runtime_error("Failed to load symbol " + std::string(symbol_name) + ": " +
                                     std::string(dlerror()));
        }
    }

    std::vector<std::string> splitString(const std::string &str, const std::string &delimiter);
};
