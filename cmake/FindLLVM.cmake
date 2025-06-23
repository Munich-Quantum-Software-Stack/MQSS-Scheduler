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
# LLVM Headers and Library
# ------------------------------------------------------------------------------
if(NOT LLVM_ROOT)
    set(LLVM_ROOT "/usr" CACHE PATH "Path to LLVM root install")
endif()
find_path(LLVM_INCLUDE_DIR
    NAMES llvm/IR/LLVMContext.h
    PATHS /usr/include)
find_library(LLVM_LIBRARY 
    NAMES LLVM 
    PATHS /usr/lib/llvm-18/lib)

# ------------------------------------------------------------------------------
# Check If LLVM Found
# ------------------------------------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVM DEFAULT_MSG LLVM_LIBRARY LLVM_INCLUDE_DIR)
message(STATUS "LLVM_LIBRARY        = ${LLVM_LIBRARY}")
message(STATUS "LLVM_INCLUDE_DIR    = ${LLVM_INCLUDE_DIR}")
if(LLVM_FOUND)
    set(LLVM_LIBRARY ${SUBMITTER_LIBRARY})
    set(LLVM_INCLUDE_DIR ${SUBMITTER_INCLUDE_DIR})
endif()