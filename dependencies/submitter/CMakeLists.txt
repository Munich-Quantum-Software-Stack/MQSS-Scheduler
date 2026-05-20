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

cmake_minimum_required(VERSION 3.19...4.0)

project(
  submitter
  VERSION 1.1.0
  LANGUAGES C CXX)

# Add C++ language support
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include external dependencies and find packages
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Find the required packages
include(cmake/ExternalDependencies.cmake)
find_package(FakeQDMI_Dev REQUIRED)
find_package(Qinfo REQUIRED)
find_package(Threads REQUIRED)
find_package(LLVM REQUIRED CONFIG)

# Compile the submitter
message(STATUS "[CMAKE-CHECK] FAKEQDMI_DEV_LIBRARIES: ${FAKEQDMI_DEV_LIBRARIES}")
file(GLOB SUBMITTER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
add_library(submitter SHARED ${SUBMITTER_SRC})
set_target_properties(submitter PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(submitter PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${QDMI_INCLUDE_DIR} ${FAKEQDMI_DEV_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS})
target_link_libraries(submitter PUBLIC LLVM Threads::Threads ${QINFO_LIBRARIES} ${FAKEQDMI_DEV_LIBRARIES})

# Set definitions for LLVM
add_definitions(${LLVM_DEFINITIONS})

# Add the test
enable_testing()
add_subdirectory(test)

# Set the installation
install(TARGETS submitter
    LIBRARY DESTINATION lib
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION include
)
