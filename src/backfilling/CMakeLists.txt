find_package(submitter REQUIRED)

find_path(ONNX_RUNTIME_SESSION_INCLUDE_DIRS onnxruntime_cxx_api.h HINTS /home/ubuntu/onnxruntime/include/onnxruntime/core/session)
find_library(ONNX_RUNTIME_LIB onnxruntime HINTS /home/ubuntu/onnxruntime/build/Linux/Release)

target_link_libraries(backfilling PUBLIC qdmi qinfo LLVM submitter ${ONNX_RUNTIME_LIB} )

target_sources(backfilling PUBLIC scheduler.cpp eval.cpp predictor.cpp queue.cpp)

target_include_directories(backfilling PUBLIC SUBMITTER_INCLUDE_DIRS ${ONNX_RUNTIME_SESSION_INCLUDE_DIRS})
