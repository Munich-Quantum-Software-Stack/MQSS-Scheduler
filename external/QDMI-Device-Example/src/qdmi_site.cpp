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

size_t qdmi::Site::get_id() const {
    size_t id{};
    // QDMI_SITE_PROPERTY_ID was renamed to QDMI_SITE_PROPERTY_INDEX as of
    // QDMI v1.2.x — same concept ("unique index/ID to identify the site in
    // a program"), name change only.
    callFuncAndThrowOnError(QDMI_device_query_site_property_ptr, "Failed to query site ID", device,
                            site, QDMI_SITE_PROPERTY_INDEX, sizeof(size_t), &id, nullptr);
    return id;
}

double qdmi::Site::get_t1() const {
    double t1{};
    callFuncAndThrowOnError(QDMI_device_query_site_property_ptr, "Failed to query site T1", device,
                            site, QDMI_SITE_PROPERTY_T1, sizeof(double), &t1, nullptr);
    return t1;
}

double qdmi::Site::get_t2() const {
    double t2{};
    callFuncAndThrowOnError(QDMI_device_query_site_property_ptr, "Failed to query site T2", device,
                            site, QDMI_SITE_PROPERTY_T2, sizeof(double), &t2, nullptr);
    return t2;
}
