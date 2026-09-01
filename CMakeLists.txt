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

# Set required cmake version
cmake_minimum_required(VERSION 3.19...4.0)
project(
  scheduler
  VERSION 1.1.0
  LANGUAGES CXX)

# Add C++ language support and enable building shared libs
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-g -DDEBUG")
set(BUILD_SHARED_LIBS ON)

# Tests and install rules default set OFF when built standalone when pulled
# in by a parent project (add_subdirectory()/FetchContent). In case building
# as top-level, lets set them ON.
option(BUILD_SCHEDULER_TESTS "Build the Scheduler test suite" OFF)
option(SCHEDULER_INSTALL "Generate install rules for the Scheduler library" OFF)
option(BUILD_SCHEDULER_DOCS "Build Doxygen API documentation" OFF)

# Include external dependencies and find packages
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Scheduler<TaskType> (include/scheduler/scheduler.hpp) is header-only 
# it only needs GoogleTest (fetched below, via
# cmake/ExternalDependencies.cmake) for tests/gtest_scheduler.cpp and
# GoogleTest itself is only fetched when tests are actually being built.
if(BUILD_SCHEDULER_TESTS)
  include(cmake/ExternalDependencies.cmake)
endif()
find_package(Threads REQUIRED)

# Compile the scheduler library
file(GLOB SCHEDULER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
add_library(scheduler SHARED ${SCHEDULER_SRC})
set_target_properties(scheduler PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(scheduler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(scheduler PUBLIC Threads::Threads)

# Add the test
if(BUILD_SCHEDULER_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(BUILD_SCHEDULER_DOCS)
  add_subdirectory(docs)
endif()

# Set the installation
if(SCHEDULER_INSTALL)
  install(TARGETS scheduler
      LIBRARY DESTINATION lib
      INCLUDES DESTINATION include
  )
  # INCLUDES DESTINATION above only tags metadata on the exported target for
  # downstream find_package(scheduler CONFIG) consumers - it does not copy any
  # header files (same gap as external/submitter/CMakeLists.txt had), so
  # the headers need installing explicitly too.
  install(DIRECTORY include/
      DESTINATION include
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.tpp"
  )
endif()
