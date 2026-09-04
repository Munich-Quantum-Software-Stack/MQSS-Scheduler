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
# FindQDMIDeviceExample.cmake
#
# Finds the QDMI-Device-Example library (local C++ QDMI binding) and the QDMI C
# headers, both installed under external/installed/.
#
# Imported variables: QDMI_DEVICE_EXAMPLE_FOUND        - True if found
# QDMI_DEVICE_EXAMPLE_INCLUDE_DIR  - Directory containing qdmi.hpp
# QDMI_DEVICE_EXAMPLE_LIBRARIES    - Full path to libqdmi_cpp QDMI_INCLUDE_DIR -
# Directory containing qdmi/client.h
# ------------------------------------------------------------------------------

# Default root: external/installed/ (relative to this file's location)
if(NOT QDMI_DEVICE_EXAMPLE_ROOT)
  set(QDMI_DEVICE_EXAMPLE_ROOT
      "${CMAKE_CURRENT_LIST_DIR}/../external/installed"
      CACHE PATH "Path to QDMI-Device-Example (and QDMI) install prefix")
endif()

# QDMI C headers
find_path(
  QDMI_INCLUDE_DIR
  NAMES qdmi/client.h
  PATHS "${QDMI_DEVICE_EXAMPLE_ROOT}/include"
  NO_DEFAULT_PATH)

# QDMI-Device-Example C++ headers
find_path(
  QDMI_DEVICE_EXAMPLE_INCLUDE_DIR
  NAMES qdmi.hpp
  PATHS "${QDMI_DEVICE_EXAMPLE_ROOT}/include"
  NO_DEFAULT_PATH)

# QDMI-Device-Example shared library
find_library(
  QDMI_DEVICE_EXAMPLE_LIBRARY
  NAMES qdmi_cpp
  PATHS "${QDMI_DEVICE_EXAMPLE_ROOT}/lib" "${QDMI_DEVICE_EXAMPLE_ROOT}/lib64"
  NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  QDMIDeviceExample
  REQUIRED_VARS QDMI_DEVICE_EXAMPLE_LIBRARY QDMI_DEVICE_EXAMPLE_INCLUDE_DIR
                QDMI_INCLUDE_DIR)

if(QDMI_DEVICE_EXAMPLE_FOUND)
  set(QDMI_DEVICE_EXAMPLE_INCLUDE_DIRS ${QDMI_DEVICE_EXAMPLE_INCLUDE_DIR}
                                       ${QDMI_INCLUDE_DIR})
  set(QDMI_DEVICE_EXAMPLE_LIBRARIES ${QDMI_DEVICE_EXAMPLE_LIBRARY})
endif()

mark_as_advanced(QDMI_INCLUDE_DIR QDMI_DEVICE_EXAMPLE_INCLUDE_DIR
                 QDMI_DEVICE_EXAMPLE_LIBRARY)
message(STATUS "QDMI_DEVICE_EXAMPLE_ROOT        = ${QDMI_DEVICE_EXAMPLE_ROOT}")
message(STATUS "QDMI_INCLUDE_DIR                = ${QDMI_INCLUDE_DIR}")
message(
  STATUS "QDMI_DEVICE_EXAMPLE_INCLUDE_DIR = ${QDMI_DEVICE_EXAMPLE_INCLUDE_DIR}")
message(
  STATUS "QDMI_DEVICE_EXAMPLE_LIBRARY     = ${QDMI_DEVICE_EXAMPLE_LIBRARY}")
