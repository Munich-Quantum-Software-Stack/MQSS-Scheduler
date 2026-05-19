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

# FindFakeQDMI_Dev.cmake
# Locates the FakeQDMI-Device-Example C++ QDMI device binding.
# Pass -DFAKEQDMI_DEV_ROOT=<prefix> and -DQDMI_ROOT=<prefix> to cmake to override.
if(NOT FAKEQDMI_DEV_ROOT)
    set(FAKEQDMI_DEV_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../installed" CACHE PATH
        "Path to FakeQDMI-Device-Example install prefix")
endif()
if(NOT QDMI_ROOT)
    set(QDMI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../installed" CACHE PATH
        "Path to QDMI install prefix")
endif()
set(CMAKE_PREFIX_PATH "${QDMI_ROOT};${FAKEQDMI_DEV_ROOT};${CMAKE_PREFIX_PATH}")

# QDMI headers
find_path(QDMI_INCLUDE_DIR
    NAMES qdmi/client.h
    PATHS ${QDMI_ROOT}/include
)

# FakeQDMI-Device-Example headers and lib
find_path(FAKEQDMI_DEV_INCLUDE_DIR
    NAMES qdmi.hpp
    PATHS ${FAKEQDMI_DEV_ROOT}/include
)

find_library(FAKEQDMI_DEV_LIBRARY
    NAMES qdmi_cpp
    PATHS ${FAKEQDMI_DEV_ROOT}/lib
)

message(STATUS "QDMI_ROOT               = ${QDMI_ROOT}")
message(STATUS "FAKEQDMI_DEV_ROOT       = ${FAKEQDMI_DEV_ROOT}")
message(STATUS "QDMI_INCLUDE_DIR        = ${QDMI_INCLUDE_DIR}")
message(STATUS "FAKEQDMI_DEV_INCLUDE_DIR = ${FAKEQDMI_DEV_INCLUDE_DIR}")
message(STATUS "FAKEQDMI_DEV_LIBRARY     = ${FAKEQDMI_DEV_LIBRARY}")

# Check if found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FakeQDMI_Dev
    REQUIRED_VARS FAKEQDMI_DEV_LIBRARY FAKEQDMI_DEV_INCLUDE_DIR QDMI_INCLUDE_DIR
)

if(FAKEQDMI_DEV_FOUND)
    set(FAKEQDMI_DEV_INCLUDE_DIRS ${FAKEQDMI_DEV_INCLUDE_DIR} ${QDMI_INCLUDE_DIR})
    set(FAKEQDMI_DEV_LIBRARIES ${FAKEQDMI_DEV_LIBRARY})
endif()

mark_as_advanced(QDMI_INCLUDE_DIR FAKEQDMI_DEV_INCLUDE_DIR FAKEQDMI_DEV_LIBRARY)
