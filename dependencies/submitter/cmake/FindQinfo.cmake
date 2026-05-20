# ------------------------------------------------------------------------------
# Copyright 2024 - 2026 Munich Quantum Software Stack
# All rights reserved.
#
# Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://llvm.org/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

# FindQinfo.cmake
# Pass -DQINFO_ROOT=<prefix> to cmake to override the default search path.
if(NOT QINFO_ROOT)
    set(QINFO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../installed" CACHE PATH
        "Path to QInfo install prefix")
endif()

# Qinfo headers
find_path(QINFO_INCLUDE_DIR
    NAMES qinfo/qinfo.h
    PATHS ${QINFO_ROOT}/include
)

# Qinfo lib (static or shared)
find_library(QINFO_LIBRARY
    NAMES qinfo
    PATHS ${QINFO_ROOT}/lib ${QINFO_ROOT}/lib64
)

message(STATUS "QINFO_ROOT          = ${QINFO_ROOT}")
message(STATUS "QINFO_INCLUDE_DIR   = ${QINFO_INCLUDE_DIR}")
message(STATUS "QINFO_LIBRARY       = ${QINFO_LIBRARY}")

# Check if found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qinfo
    REQUIRED_VARS QINFO_INCLUDE_DIR QINFO_LIBRARY
)

if(QINFO_FOUND)
    set(QINFO_INCLUDE_DIRS ${QINFO_INCLUDE_DIR})
    set(QINFO_LIBRARIES ${QINFO_LIBRARY})
endif()

mark_as_advanced(QINFO_INCLUDE_DIR QINFO_LIBRARY)
