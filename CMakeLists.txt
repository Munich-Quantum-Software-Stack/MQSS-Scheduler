cmake_minimum_required(VERSION 3.22)

project(QuantumScheduler
        VERSION 0.1
        DESCRIPTION "Quantum Scheduler")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-g")

#add_library(ROUND_ROBIN STATIC)
add_library(HEURISTIC STATIC)

#target_include_directories(ROUND_ROBIN PUBLIC include/round_robin include/)
target_include_directories(HEURISTIC PUBLIC include/heuristic)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})

#include_directories(/home/ubuntu/bin/include)
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(src)
add_subdirectory(tests)

