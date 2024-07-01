include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
    onnxruntime
    GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
    GIT_TAG v1.18.1
)

FetchContent_MakeAvailable(onnxruntime)

# Build ONNX Runtime using its build script
execute_process(
    COMMAND bash build.sh --config Release --build_shared_lib --parallel --update --build
    WORKING_DIRECTORY ${onnxruntime_SOURCE_DIR}
    RESULT_VARIABLE ONNXRUNTIME_BUILD_RESULT
)

# Check if ONNX Runtime built successfully
if(NOT ONNXRUNTIME_BUILD_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to build ONNX Runtime")
endif()

target_include_directories(backfilling PUBLIC 
    ${SUBMITTER_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/build/_deps/onnxruntime-src/include/onnxruntime/core/session/
)

set(ONNXRUNTIME_LIB_DIR ${CMAKE_SOURCE_DIR}/build/_deps/onnxruntime-src/build/Linux/Release)

target_link_libraries(backfilling 
    PUBLIC 
    ${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so
    ${ONNXRUNTIME_LIB_DIR}/libonnxruntime_providers_shared.so
    submitter
)

target_sources(backfilling PUBLIC scheduler.cpp eval.cpp predictor.cpp queue.cpp)