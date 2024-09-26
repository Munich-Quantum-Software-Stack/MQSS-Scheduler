target_include_directories(queue PUBLIC ${SUBMITTER_INCLUDE_DIRS})

target_link_libraries(queue PUBLIC submitter)

target_sources(queue PRIVATE queue.cpp)