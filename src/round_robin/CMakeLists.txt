find_package(submitter REQUIRED)

target_include_directories(round_robin PUBLIC SUBMITTER_INCLUDE_DIRS) 
#target_include_directories(round_robin PUBLIC ${CMAKE_BINARY_DIR}/_deps/qdmi-src/include) # do better
#target_include_directories(round_robin PUBLIC ${CMAKE_BINARY_DIR}/_deps/qinfo-src/include) # do better
#target_include_directories(round_robin PUBLIC ${CMAKE_BINARY_DIR}/_deps/fomac-src/include) # do better

target_link_libraries(round_robin PRIVATE submitter)

target_sources(round_robin PRIVATE scheduler_round_robin.cpp)
