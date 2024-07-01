add_executable(schedulerBackfillingTest schedulerBackfillingTest.cpp)

target_link_libraries(schedulerBackfillingTest backfilling LLVM)

target_include_directories(schedulerBackfillingTest PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/build/_deps/submitter-src/include
    ${CMAKE_SOURCE_DIR}/build/_deps/qdmi-src/include
    ${CMAKE_SOURCE_DIR}/build/_deps/qinfo-src/include
)

add_test(NAME schedulerBackfillingTest COMMAND schedulerBackfillingTest)