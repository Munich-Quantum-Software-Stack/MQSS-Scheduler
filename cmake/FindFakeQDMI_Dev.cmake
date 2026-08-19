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

# ------------------------------------------------------------------------------
# FindFakeQDMI_Dev.cmake
#
# Finds the FakeQDMI-Device-Example library (local C++ QDMI binding) and the
# QDMI C headers, both installed under external/installed/.
#
# Imported variables:
#   FAKEQDMI_DEV_FOUND        - True if found
#   FAKEQDMI_DEV_INCLUDE_DIR  - Directory containing qdmi.hpp
#   FAKEQDMI_DEV_LIBRARIES    - Full path to libqdmi_cpp
#   QDMI_INCLUDE_DIR          - Directory containing qdmi/client.h
# ------------------------------------------------------------------------------

# Default root: external/installed/ (relative to this file's location)
if(NOT FAKEQDMI_DEV_ROOT)
    set(FAKEQDMI_DEV_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/../external/installed"
        CACHE PATH "Path to FakeQDMI-Dev (and QDMI) install prefix")
endif()

# QDMI C headers
find_path(QDMI_INCLUDE_DIR
    NAMES qdmi/client.h
    PATHS "${FAKEQDMI_DEV_ROOT}/include"
    NO_DEFAULT_PATH
)

# FakeQDMI-Dev C++ headers
find_path(FAKEQDMI_DEV_INCLUDE_DIR
    NAMES qdmi.hpp
    PATHS "${FAKEQDMI_DEV_ROOT}/include"
    NO_DEFAULT_PATH
)

# FakeQDMI-Dev shared library
find_library(FAKEQDMI_DEV_LIBRARY
    NAMES qdmi_cpp
    PATHS "${FAKEQDMI_DEV_ROOT}/lib"
          "${FAKEQDMI_DEV_ROOT}/lib64"
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FakeQDMI_Dev
    REQUIRED_VARS FAKEQDMI_DEV_LIBRARY FAKEQDMI_DEV_INCLUDE_DIR QDMI_INCLUDE_DIR
)

if(FAKEQDMI_DEV_FOUND)
    set(FAKEQDMI_DEV_INCLUDE_DIRS ${FAKEQDMI_DEV_INCLUDE_DIR} ${QDMI_INCLUDE_DIR})
    set(FAKEQDMI_DEV_LIBRARIES ${FAKEQDMI_DEV_LIBRARY})
endif()

mark_as_advanced(QDMI_INCLUDE_DIR FAKEQDMI_DEV_INCLUDE_DIR FAKEQDMI_DEV_LIBRARY)
message(STATUS "FAKEQDMI_DEV_ROOT        = ${FAKEQDMI_DEV_ROOT}")
message(STATUS "QDMI_INCLUDE_DIR         = ${QDMI_INCLUDE_DIR}")
message(STATUS "FAKEQDMI_DEV_INCLUDE_DIR = ${FAKEQDMI_DEV_INCLUDE_DIR}")
message(STATUS "FAKEQDMI_DEV_LIBRARY     = ${FAKEQDMI_DEV_LIBRARY}")
