
#set(CMAKE_PREFIX_PATH "/home/ubuntu/bin/cmake/" ${CMAKE_PREFIX_PATH})
#message($CMAKE_MODULE_PATH)


#find_package(qinfo REQUIRED CONFIG)
#find_package(qdmi REQUIRED CONFIG)
#find_package(fomac REQUIRED)


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake" ${CMAKE_MODULE_PATH})
find_package(qdmi REQUIRED)
find_package(qinfo REQUIRED)
find_package(fomac REQUIRED)
find_package(submitter REQUIRED)

#message(${CMAKE_BINARY_DIR}/_deps/fomac-src/include)

target_include_directories(HEURISTIC PUBLIC ${CMAKE_BINARY_DIR}/_deps/qdmi-src/include) # do better
target_include_directories(HEURISTIC PUBLIC ${CMAKE_BINARY_DIR}/_deps/qinfo-src/include) # do better
target_include_directories(HEURISTIC PUBLIC ${CMAKE_BINARY_DIR}/_deps/fomac-src/include) # do better
target_include_directories(HEURISTIC PUBLIC ${CMAKE_BINARY_DIR}/_deps/submitter-src/include) # do better
target_include_directories(HEURISTIC PUBLIC /home/ubuntu/onnxruntime/include) # do better

find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime PATHS /home/ubuntu/onnxruntime/build/Linux/Release)


target_link_libraries(HEURISTIC PRIVATE qdmi qinfo fomac LLVM  SUBMITTER ${ONNXRUNTIME_LIBRARY} )
#submitter 

target_sources(HEURISTIC PRIVATE scheduler_heuristic.cpp eval.cpp predictor.cpp)
