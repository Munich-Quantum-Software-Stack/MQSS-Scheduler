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

# Expose Google Test and spdlog
include(GoogleTest)

# Compile the tests
add_executable(gtest_scheduler gtest_scheduler.cpp)
target_include_directories(gtest_scheduler PRIVATE ${QDMI_INCLUDE_DIR} ${CXXQDMI_INCLUDE_DIR})
target_include_directories(gtest_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(gtest_scheduler PRIVATE scheduler submitter LLVM Threads::Threads ${CXXQDMI_LIBRARIES})
target_link_libraries(gtest_scheduler PUBLIC GTest::gtest_main pthread)

# Register the test
add_test(NAME Scheduler_QDMI_Test COMMAND gtest_scheduler)
# message(STATUS "[CMAKE-CHECK] QDMI_INCLUDE_BUILD_DIR: ${QDMI_INCLUDE_BUILD_DIR}")
# message(STATUS "[CMAKE-CHECK] qdmi_BINARY_DIR: ${qdmi_BINARY_DIR}")
# message(STATUS "[CMAKE-CHECK] CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

set_tests_properties(
    Scheduler_QDMI_Test
    PROPERTIES
    ENVIRONMENT
    "LD_LIBRARY_PATH=/home/admin/shared/QDMI/build/examples/driver:/home/admin/shared/QInfo/install/lib:/home/admin/shared/submitter/install/lib:/home/admin/shared/scheduler/build:/home/admin/shared/CxxQDMI/install/lib;QDMI_CONF=/home/admin/shared/CxxQDMI/build/tests/qdmi.conf"
)

