find_package(submitter REQUIRED)

target_include_directories(heuristic PUBLIC SUBMITTER_INCLUDE_DIRS) 

find_path(ONNX_RUNTIME_SESSION_INCLUDE_DIRS onnxruntime_cxx_api.h HINTS /usr/local/include/onnxruntime/core/session/)
find_library(ONNX_RUNTIME_LIB onnxruntime HINTS /usr/local/lib)

target_link_libraries(heuristic PUBLIC qdmi qinfo LLVM submitter ${ONNX_RUNTIME_LIB} )

target_sources(heuristic PUBLIC scheduler_heuristic.cpp eval.cpp predictor.cpp)
