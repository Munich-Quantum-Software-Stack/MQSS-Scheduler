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
# FindCxxQDMI.cmake
# ------------------------------------------------------------------------------
set(QDMI_ROOT "/home/admin/shared/QDMI/install")
set(CXXQDMI_ROOT "/home/admin/shared/CxxQDMI/install")
set(CMAKE_PREFIX_PATH "${QDMI_ROOT};${CXXQDMI_ROOT};${CMAKE_PREFIX_PATH}")

# ------------------------------------------------------------------------------
# QDMI headers
# ------------------------------------------------------------------------------
find_path(QDMI_INCLUDE_DIR
    NAMES qdmi/client.h
    PATHS ${QDMI_ROOT}/include
)

# ------------------------------------------------------------------------------
# CxxQDMI headers and lib
# ------------------------------------------------------------------------------
find_path(CXXQDMI_INCLUDE_DIR
    NAMES qdmi.hpp
    PATHS ${CXXQDMI_ROOT}/include
)
find_library(CXXQDMI_LIBRARY
    NAMES qdmi_cpp
    PATHS ${CXXQDMI_ROOT}/lib
)

# ------------------------------------------------------------------------------
# Check if found
# ------------------------------------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CxxQDMI
    REQUIRED_VARS CXXQDMI_LIBRARY CXXQDMI_INCLUDE_DIR QDMI_INCLUDE_DIR
)

if(CXXQDMI_FOUND)
    set(CXXQDMI_INCLUDE_DIRS ${CXXQDMI_INCLUDE_DIR} ${QDMI_INCLUDE_DIR})
    set(CXXQDMI_LIBRARIES ${CXXQDMI_LIBRARY})
endif()

mark_as_advanced(QDMI_INCLUDE_DIR CXXQDMI_INCLUDE_DIR CXXQDMI_LIBRARY)
message(STATUS "QDMI_ROOT           = ${QDMI_ROOT}")
message(STATUS "QDMI_INCLUDE_DIR    = ${QDMI_INCLUDE_DIR}")
message(STATUS "CXXQDMI_ROOT        = ${CXXQDMI_ROOT}")
message(STATUS "CXXQDMI_INCLUDE_DIR = ${CXXQDMI_INCLUDE_DIR}")
message(STATUS "CXXQDMI_LIBRARY     = ${CXXQDMI_LIBRARY}")
