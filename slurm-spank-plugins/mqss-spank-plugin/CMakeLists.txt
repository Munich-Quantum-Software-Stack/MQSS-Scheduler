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

cmake_minimum_required(VERSION 3.28)
project(mqss-spank-plugin-testbed LANGUAGES CXX)

option(BUILD_MQSS_SPANK "Build the MQSS SPANK plugin" OFF)

if(BUILD_MQSS_SPANK)
  find_path(SLURM_SPANK_INCLUDE_DIR NAMES slurm/spank.h)
  if(NOT SLURM_SPANK_INCLUDE_DIR)
    message(FATAL_ERROR "Could not locate slurm/spank.h (install libslurm-dev)")
  endif()

  set(BUILD_MQSS_SPANK_TARGET mqss-spank-plugin)
  add_library(${BUILD_MQSS_SPANK_TARGET} MODULE mqss-spank-plugin.cpp)
  target_compile_features(${BUILD_MQSS_SPANK_TARGET} PRIVATE cxx_std_20)
  target_include_directories(${BUILD_MQSS_SPANK_TARGET}
                             PRIVATE ${SLURM_SPANK_INCLUDE_DIR})
  set_target_properties(${BUILD_MQSS_SPANK_TARGET} PROPERTIES PREFIX "")

  # Detect the Slurm plugin directory for the SPANK plugin.
  set(MQSS_SPANK_INSTALL_DIR_DEFAULT "/usr/lib/slurm")
  find_program(MQSS_SPANK_SCONTROL_EXECUTABLE scontrol)
  if(MQSS_SPANK_SCONTROL_EXECUTABLE)
    execute_process(
      COMMAND ${MQSS_SPANK_SCONTROL_EXECUTABLE} show config
      OUTPUT_VARIABLE MQSS_SPANK_SLURM_CONFIG
      RESULT_VARIABLE MQSS_SPANK_SCONTROL_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    if(MQSS_SPANK_SCONTROL_RESULT EQUAL 0)
      string(REGEX MATCH "PluginDir[ \t]*=[ \t]*([^ \t\r\n]+)"
                   MQSS_SPANK_PLUGIN_DIR_MATCH "${MQSS_SPANK_SLURM_CONFIG}")
      if(CMAKE_MATCH_1)
        set(MQSS_SPANK_INSTALL_DIR_DEFAULT "${CMAKE_MATCH_1}")
      endif()
    endif()
  endif()

  set(MQSS_SPANK_INSTALL_DIR
      "${MQSS_SPANK_INSTALL_DIR_DEFAULT}"
      CACHE
        PATH
        "Install directory for the MQSS SPANK plugin (override for distro-specific Slurm plugin paths)"
  )

  mark_as_advanced(MQSS_SPANK_SCONTROL_EXECUTABLE)

  # Detect the Slurm configuration directory for the plugstack.conf.d drop-in.
  set(MQSS_SPANK_SLURM_CONF_DIR_DEFAULT "/etc/slurm")
  if(MQSS_SPANK_SCONTROL_EXECUTABLE AND MQSS_SPANK_SCONTROL_RESULT EQUAL 0)
    string(REGEX MATCH "PlugStackConfig[ \t]*=[ \t]*([^ \t\r\n]+)"
                 MQSS_SPANK_PLUGSTACK_MATCH "${MQSS_SPANK_SLURM_CONFIG}")
    if(CMAKE_MATCH_1)
      get_filename_component(MQSS_SPANK_SLURM_CONF_DIR_DEFAULT "${CMAKE_MATCH_1}"
                             DIRECTORY)
    endif()
  endif()

  set(MQSS_SPANK_SLURM_CONF_DIR
      "${MQSS_SPANK_SLURM_CONF_DIR_DEFAULT}"
      CACHE PATH "Slurm configuration directory (for plugstack.conf.d drop-in)")

  # Generate the plugstack drop-in configuration with the correct plugin path.
  configure_file(mqss-spank.conf.in mqss-spank.conf @ONLY)

  install(TARGETS ${BUILD_MQSS_SPANK_TARGET}
          LIBRARY DESTINATION ${MQSS_SPANK_INSTALL_DIR}
                  COMPONENT mqss-spank-plugin)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/mqss-spank.conf
    DESTINATION ${MQSS_SPANK_SLURM_CONF_DIR}/plugstack.conf.d
    COMPONENT mqss-spank-plugin)
endif()
