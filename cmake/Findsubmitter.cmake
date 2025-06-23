# ------------------------------------------------------------------------------
# Copyright 2024 Munich Quantum Software Stack Project
#
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
# "License"); you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/develop/LICENSE
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
# ------------------------------------------------------------------------------
set(SUBMITTER_ROOT "/home/admin/shared/submitter/install")
set(CMAKE_PREFIX_PATH "${SUBMITTER_ROOT};${CMAKE_PREFIX_PATH}")

# ------------------------------------------------------------------------------
# Submitter Headers and Libs
# ------------------------------------------------------------------------------
find_path(SUBMITTER_INCLUDE_DIR
    NAMES submitter.hpp
    PATHS ${SUBMITTER_ROOT}/include
)
find_library(SUBMITTER_LIBRARY
    NAMES submitter
    PATHS ${SUBMITTER_ROOT}/lib
)

# ------------------------------------------------------------------------------
# Check If Submitter Found
# ------------------------------------------------------------------------------
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
