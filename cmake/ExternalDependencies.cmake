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

include(FetchContent)
set(FETCH_DEPENDENT_PACKAGES "")

# ------------------------------------------------------------------------------
# Google Test — use a pre-installed version if available and possible, fetch and
# build the pinned GIT_TAG below otherwise.
# ------------------------------------------------------------------------------
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  FIND_PACKAGE_ARGS NAMES GTest)
list(APPEND FETCH_DEPENDENT_PACKAGES googletest)

# GoogleTest doesn't need to be a shared library just because this project
# defaults to one (BUILD_SHARED_LIBS ON, set in the CMakeLists.txt file for the
# Scheduler library itself, and inherited by subdirectory added afterward
# including this FetchContent population)
#
# gtest_main is only ever linked into the private tests/gtest_scheduler
# executable, never installed or used as a runtime dependency elsewhere. We can
# force it static for just this one dependency. Static linking sidesteps it
# entirely.
set(_saved_build_shared_libs "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(${FETCH_DEPENDENT_PACKAGES})
set(BUILD_SHARED_LIBS "${_saved_build_shared_libs}")
unset(_saved_build_shared_libs)
