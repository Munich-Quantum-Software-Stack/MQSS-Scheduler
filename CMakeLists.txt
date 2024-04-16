cmake_minimum_required(VERSION 3.22.1)

project(QuantumScheduler LANGUAGES CXX VERSION 0.1 DESCRIPTION "Quantum Scheduler")

set(CMAKE_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(INSTALL_PATH "${CMAKE_INSTALL_PREFIX}")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set LLVM_ENABLE_THREADS to ON
set(LLVM_ENABLE_THREADS ON)

# Find LLVMConfig.cmake
find_path(LLVM_CONFIG_DIR LLVMConfig.cmake HINTS "${CMAKE_PREFIX_PATH}" ENV
  LLVM_DIR)

if(NOT LLVM_CONFIG_DIR)
  message(
    FATAL_ERROR "LLVMConfig.cmake not found. Set CMAKE_PREFIX_PATH or LLVM_DIR."
  )
endif()

# Include LLVMConfig.cmake
list(APPEND CMAKE_PREFIX_PATH "${LLVM_CONFIG_DIR}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_CONFIG_DIR}")
find_package(LLVM REQUIRED CONFIG)

# Set up LLVM-specific flags
add_definitions(${LLVM_DEFINITIONS})

# Set up include directories
add_subdirectory(src)

# Build the documentation
option(BUILD_WITH_DOCS "Generate Docs" OFF)

if(BUILD_WITH_DOCS)
  set(BUILD_DOCS_STANDALONE OFF)
  message("Building documentation.")
  set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/docs)
endif()

# Setup tests
enable_testing()
add_subdirectory(tests)
