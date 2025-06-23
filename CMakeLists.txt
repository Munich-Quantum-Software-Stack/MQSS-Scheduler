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

# Set required cmake version
cmake_minimum_required(VERSION 3.19...4.0)
project(
  scheduler
  VERSION 1.1.0
  LANGUAGES CXX)

# Add C++ language support and enable building shared libs
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-g")
set(BUILD_SHARED_LIBS ON)

# Include external dependencies and find packages
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(cmake/ExternalDependencies.cmake)
find_package(submitter REQUIRED)
find_package(CxxQDMI REQUIRED)
find_package(LLVM REQUIRED)
find_package(Threads REQUIRED)

# Compile the scheduler library
file(GLOB SCHEDULER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/queue/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
add_library(scheduler SHARED ${SCHEDULER_SRC})
set_target_properties(scheduler PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(scheduler PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${QDMI_INCLUDE_DIR} ${CXXQDMI_INCLUDE_DIR} ${SUBMITTER_INCLUDE_DIR})
target_link_libraries(scheduler PUBLIC LLVM Threads::Threads Qinfo ${CXXQDMI_LIBRARIES} ${SUBMITTER_LIBRARIES})

# Add the test
enable_testing()
add_subdirectory(tests)

# Set the installation
install(TARGETS scheduler
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
)

