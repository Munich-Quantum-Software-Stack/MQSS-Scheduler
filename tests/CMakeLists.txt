# Copyright (c) 2024 - 2026 Munich Quantum Software Stack
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

message(STATUS "Running tests from: ${CMAKE_CURRENT_SOURCE_DIR}")

# Add C++ language support
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Expose Google Test
include(GoogleTest)

# Scheduler is header-only (include/scheduler/scheduler.hpp + .tpp,
# include/scheduler/quantum_task.hpp) with no QDMI/LLVM/submitter/spdlog
# dependency, so this test target needs nothing but GTest and the repo's
# include/ directory - no LD_LIBRARY_PATH or QDMI_CONF environment setup
# required to run it.
add_executable(gtest_scheduler gtest_scheduler.cpp)
target_include_directories(gtest_scheduler PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(gtest_scheduler PRIVATE GTest::gtest_main Threads::Threads)

add_test(NAME Scheduler_Test COMMAND gtest_scheduler)
