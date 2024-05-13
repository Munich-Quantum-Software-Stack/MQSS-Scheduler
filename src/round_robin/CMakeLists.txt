
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake" ${CMAKE_MODULE_PATH})
find_package(qdmi REQUIRED)
find_package(qinfo REQUIRED)
find_package(fomac REQUIRED)

target_include_directories(ROUND_ROBIN PUBLIC ${CMAKE_BINARY_DIR}/_deps/qdmi-src/include) # do better
target_include_directories(ROUND_ROBIN PUBLIC ${CMAKE_BINARY_DIR}/_deps/qinfo-src/include) # do better
target_include_directories(ROUND_ROBIN PUBLIC ${CMAKE_BINARY_DIR}/_deps/fomac-src/include) # do better

target_link_libraries(ROUND_ROBIN PRIVATE qdmi qinfo fomac LLVM)

target_sources(ROUND_ROBIN PRIVATE scheduler_round_robin.cpp)
