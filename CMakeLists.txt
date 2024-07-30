cmake_minimum_required(VERSION 3.26)

project(QuantumScheduler
        VERSION 0.1
        DESCRIPTION "Quantum Scheduler")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-g")

add_library(round_robin SHARED)
add_library(backfilling SHARED)
add_library(queue SHARED)

target_include_directories(round_robin PUBLIC include/round_robin)
target_include_directories(backfilling PUBLIC include/backfilling)
target_include_directories(queue PUBLIC include)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS} include)
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(src)
add_subdirectory(tests)

