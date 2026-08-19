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
# FindSubmitter.cmake
#
# Imported variables:
#   SUBMITTER_FOUND        - True if found
#   SUBMITTER_INCLUDE_DIR  - Directory containing submitter.hpp
#   SUBMITTER_LIBRARIES    - Full path to libsubmitter
# ------------------------------------------------------------------------------

# Default root: external/installed/ (relative to this file's location)
if(NOT SUBMITTER_ROOT)
    set(SUBMITTER_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/../external/installed"
        CACHE PATH "Path to submitter install prefix")
endif()

find_path(SUBMITTER_INCLUDE_DIR
    NAMES submitter.hpp
    PATHS "${SUBMITTER_ROOT}/include"
    NO_DEFAULT_PATH
)
find_library(SUBMITTER_LIBRARY
    NAMES submitter
    PATHS "${SUBMITTER_ROOT}/lib"
          "${SUBMITTER_ROOT}/lib64"
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(submitter
    REQUIRED_VARS SUBMITTER_LIBRARY SUBMITTER_INCLUDE_DIR
)

if(SUBMITTER_FOUND)
    set(SUBMITTER_INCLUDE_DIRS ${SUBMITTER_INCLUDE_DIR})
    set(SUBMITTER_LIBRARIES ${SUBMITTER_LIBRARY})
endif()

mark_as_advanced(SUBMITTER_INCLUDE_DIR SUBMITTER_LIBRARY)
message(STATUS "SUBMITTER_ROOT        = ${SUBMITTER_ROOT}")
message(STATUS "SUBMITTER_INCLUDE_DIR = ${SUBMITTER_INCLUDE_DIR}")
message(STATUS "SUBMITTER_LIBRARY     = ${SUBMITTER_LIBRARY}")
