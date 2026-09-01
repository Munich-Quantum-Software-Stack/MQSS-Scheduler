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

find_package(Doxygen REQUIRED)
include(FetchContent)

FetchContent_Declare(
  doxygen-awesome-css
  GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
  GIT_TAG v2.3.4)
FetchContent_MakeAvailable(doxygen-awesome-css)
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

set(SCHEDULER_DOXYGEN_OUTPUT_DIR "${PROJECT_BINARY_DIR}/docs")
set(SCHEDULER_DOXYGEN_CONFIG_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
set(SCHEDULER_DOXYGEN_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
set(PROJECT_LOGO "mqss_logo.svg")

set(SCHEDULER_DOXYGEN_INPUT_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/index.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/getting-started.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/development-guide.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/api-reference.md")
string(REPLACE ";" " " SCHEDULER_DOXYGEN_INPUT_DIRS
               "${SCHEDULER_DOXYGEN_INPUT_DIRS}")

file(
  GLOB_RECURSE
  SCHEDULER_DOXYGEN_INPUT_FILES
  CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/include/*.hpp"
  "${PROJECT_SOURCE_DIR}/include/*.tpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/_static/*.svg"
  "${CMAKE_CURRENT_SOURCE_DIR}/header.html"
  "${CMAKE_CURRENT_SOURCE_DIR}/layout.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/style.css")

configure_file("${SCHEDULER_DOXYGEN_CONFIG_FILE_IN}"
               "${SCHEDULER_DOXYGEN_CONFIG_FILE}" @ONLY)

file(MAKE_DIRECTORY "${SCHEDULER_DOXYGEN_OUTPUT_DIR}/html")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/_static/mqss_logo_dark.svg"
               "${SCHEDULER_DOXYGEN_OUTPUT_DIR}/html/mqss_logo_dark.svg"
               COPYONLY)

add_custom_command(
  OUTPUT "${SCHEDULER_DOXYGEN_OUTPUT_DIR}/html/index.html"
  COMMAND "${CMAKE_COMMAND}" -E make_directory
          "${SCHEDULER_DOXYGEN_OUTPUT_DIR}/html"
  COMMAND "${DOXYGEN_EXECUTABLE}" "${SCHEDULER_DOXYGEN_CONFIG_FILE}"
  DEPENDS ${SCHEDULER_DOXYGEN_INPUT_FILES} "${SCHEDULER_DOXYGEN_CONFIG_FILE}"
          "${SCHEDULER_DOXYGEN_CONFIG_FILE_IN}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Generating Scheduler documentation with Doxygen"
  VERBATIM)

add_custom_target(docs DEPENDS "${SCHEDULER_DOXYGEN_OUTPUT_DIR}/html/index.html")
