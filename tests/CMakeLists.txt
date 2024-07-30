add_executable(schedulerBackfillingTest schedulerBackfillingTest.cpp)
target_link_libraries(schedulerBackfillingTest backfilling LLVM)
target_include_directories(schedulerBackfillingTest PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${QDMI_INCLUDE_DIRS}
    ${SUBMITTER_INCLUDE_DIRS}
)
add_test(NAME schedulerBackfillingTest COMMAND schedulerBackfillingTest)

add_executable(schedulerRoundRobinTest schedulerRoundRobinTest.cpp)
target_link_libraries(schedulerRoundRobinTest round_robin LLVM)
target_include_directories(schedulerRoundRobinTest PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${QDMI_INCLUDE_DIRS}
    ${SUBMITTER_INCLUDE_DIRS}
)
add_test(NAME schedulerRoundRobinTest COMMAND schedulerRoundRobinTest)