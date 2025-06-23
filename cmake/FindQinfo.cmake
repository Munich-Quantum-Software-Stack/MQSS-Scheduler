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
# Qinfo Headers and Library
# ------------------------------------------------------------------------------
find_path(QINFO_INCLUDE_DIR
    NAMES qinfo/qinfo.h
    PATHS /home/admin/shared/QInfo/install/include
)
find_library(QINFO_LIBRARY
    NAMES qinfo
    PATHS /home/admin/shared/QInfo/install/lib
)

# ------------------------------------------------------------------------------
# Check Qinfo Paths
# ------------------------------------------------------------------------------
message(STATUS "QINFO_INCLUDE_DIR   = ${QINFO_INCLUDE_DIR}")
message(STATUS "QINFO_LIBRARY       = ${QINFO_LIBRARY}")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qinfo
    REQUIRED_VARS QINFO_INCLUDE_DIR QINFO_LIBRARY
)

# ------------------------------------------------------------------------------
# Set Qinfo Env Variables
# ------------------------------------------------------------------------------
if(QINFO_FOUND)
    set(QINFO_INCLUDE_DIR ${QINFO_INCLUDE_DIR})
    set(QINFO_LIBRARY ${QINFO_LIBRARY})
endif()
mark_as_advanced(QINFO_INCLUDE_DIR QINFO_LIBRARY)
