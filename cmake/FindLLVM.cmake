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

# ------------------------------------------------------------------------------
# FindLLVM.cmake
#
# Wrapper that locates the pre-built LLVM installation via its own
# LLVMConfig.cmake. Sets LLVM_DIR and delegates to the official config module so
# all LLVM imported targets (e.g. "LLVM") and variables are available.
#
# Set LLVM_ROOT to override the search location. Default is the value of the
# LLVM_ROOT environment variable, or /home/admin/shared/dependencies/installed
# (the shared LLVM pre-built by the top-level dependency scripts).
# ------------------------------------------------------------------------------

if(NOT LLVM_ROOT)
  if(DEFINED ENV{LLVM_ROOT})
    set(LLVM_ROOT
        "$ENV{LLVM_ROOT}"
        CACHE PATH "Path to LLVM root install")
  else()
    set(LLVM_ROOT
        "/home/admin/shared/dependencies/installed"
        CACHE PATH "Path to LLVM root install")
  endif()
endif()

set(LLVM_DIR
    "${LLVM_ROOT}/lib/cmake/llvm"
    CACHE PATH "Path to LLVMConfig.cmake" FORCE)

# Delegate to the official LLVM config — provides all imported targets and vars
find_package(LLVM CONFIG REQUIRED HINTS "${LLVM_DIR}" NO_CMAKE_FIND_ROOT_PATH)

message(STATUS "LLVM_ROOT         = ${LLVM_ROOT}")
message(STATUS "LLVM_DIR          = ${LLVM_DIR}")
message(STATUS "LLVM_VERSION      = ${LLVM_VERSION}")
message(STATUS "LLVM_INCLUDE_DIRS = ${LLVM_INCLUDE_DIRS}")
