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
  QDMIDeviceExample
  VERSION 1.1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS ON)

# ------------------------------------------------------------------------------
# Find QDMI (headers only needed — provides qdmi/client.h, qdmi/types.h)
# ------------------------------------------------------------------------------
if(NOT QDMI_ROOT)
  get_filename_component(QDMI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../installed" REALPATH CACHE)
endif()

find_path(QDMI_INCLUDE_DIR
  NAMES qdmi/client.h
  PATHS "${QDMI_ROOT}/include"
  REQUIRED)

message(STATUS "QDMI_INCLUDE_DIR = ${QDMI_INCLUDE_DIR}")

# ------------------------------------------------------------------------------
# Library: qdmi_cpp
# ------------------------------------------------------------------------------
file(GLOB QDMI_DEVICE_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(objqdmi_cpp OBJECT ${QDMI_DEVICE_EXAMPLE_SOURCES})
set_target_properties(objqdmi_cpp PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(objqdmi_cpp
  PRIVATE
    "${QDMI_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(qdmi_cpp_shared SHARED $<TARGET_OBJECTS:objqdmi_cpp>)
set_target_properties(qdmi_cpp_shared PROPERTIES OUTPUT_NAME qdmi_cpp)
target_include_directories(qdmi_cpp_shared
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${QDMI_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>)

add_library(qdmi_cpp_static STATIC $<TARGET_OBJECTS:objqdmi_cpp>)
set_target_properties(qdmi_cpp_static PROPERTIES OUTPUT_NAME qdmi_cpp)

add_library(qdmi_cpp ALIAS qdmi_cpp_shared)
add_library(qdmi::cpp ALIAS qdmi_cpp_shared)

# ------------------------------------------------------------------------------
# Install
# ------------------------------------------------------------------------------
install(DIRECTORY include/ DESTINATION include)
install(TARGETS qdmi_cpp_shared LIBRARY DESTINATION lib)
install(TARGETS qdmi_cpp_static ARCHIVE DESTINATION lib)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------
option(BUILD_QDMI_DEVICE_EXAMPLE_TESTS "Build QDMI-Device-Example tests" OFF)
if(BUILD_QDMI_DEVICE_EXAMPLE_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(test)
endif()
