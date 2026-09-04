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

message(STATUS "Running tests from: ${CMAKE_CURRENT_SOURCE_DIR}")

# Add C++ language support
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect the operating system
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(LIB_EXT "dylib")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(LIB_EXT "so")
else()
  message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()

# Set the path to QDMI
if(NOT QDMI_SRC_DIR)
  get_filename_component(QDMI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../QDMI" REALPATH CACHE)
  set(QDMI_BUILD_DIR "${QDMI_SRC_DIR}/build" CACHE PATH "Path to QDMI build prefix")
endif()

if(NOT QDMI_INCLUDE_DIR)
  get_filename_component(QDMI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../installed/include" REALPATH CACHE)
endif()

# Expose Google Test
include(GoogleTest)

# Compile the tests
add_executable(test_cxx_qdmi_dev test_cxx_qdmi_dev.cpp)
target_include_directories(test_cxx_qdmi_dev PRIVATE ${QDMI_INCLUDE_DIR})
target_link_libraries(test_cxx_qdmi_dev PRIVATE qdmi_cpp_static)
target_link_libraries(test_cxx_qdmi_dev PUBLIC GTest::gtest_main pthread)

# Register the test
add_test(NAME CxxQDMI_Tests COMMAND test_cxx_qdmi_dev)

set_tests_properties(
    CxxQDMI_Tests
    PROPERTIES
    ENVIRONMENT
    "LD_LIBRARY_PATH=${QDMI_BUILD_DIR}/examples/driver/;QDMI_CONF=${CMAKE_CURRENT_BINARY_DIR}/test/qdmi.conf"
)

# File needed for testing
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/qdmi.conf
    ${QDMI_BUILD_DIR}/examples/device/cxx/libcxx_device.${LIB_EXT}
    " CXX read_write")
