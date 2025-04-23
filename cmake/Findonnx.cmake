# include(FetchContent)
# include(ExternalProject)

# FetchContent_Declare(
#     onnxruntime
#     GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
#     GIT_TAG v1.18.1
# )

# FetchContent_MakeAvailable(onnxruntime)

# # Build ONNX Runtime using its build script
# execute_process(
#     COMMAND bash build.sh --config Release --build_shared_lib --parallel --update --build
#     WORKING_DIRECTORY ${onnxruntime_SOURCE_DIR}
#     RESULT_VARIABLE ONNXRUNTIME_BUILD_RESULT
# )

# # Check if ONNX Runtime built successfully
# if(NOT ONNXRUNTIME_BUILD_RESULT EQUAL "0")
#     message(FATAL_ERROR "Failed to build ONNX Runtime")
# endif()

set(ONNXRUNTIME_INCLUDE_DIRS "/usr/local/include/onnxruntime")
set(ONNXRUNTIME_LIB_DIR "/usr/local/lib")