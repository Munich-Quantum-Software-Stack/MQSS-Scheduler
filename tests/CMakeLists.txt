#add_executable(schedulerRoundRobinTest schedulerRoundRobinTest.cpp)
#target_link_libraries(schedulerRoundRobinTest ROUND_ROBIN LLVM)
#add_test(NAME schedulerRoundRobinTest COMMAND schedulerRoundRobinTest)

#add_executable(selectorAll selectorAllTest.cpp)
#target_link_libraries(selectorAll ALL)
#add_test(NAME selectorAll COMMAND selectorAll)
#find_package(Threads REQUIRED)

add_executable(schedulerHeuristicTest schedulerHeuristicTest.cpp)
target_link_libraries(schedulerHeuristicTest HEURISTIC LLVM)
add_test(NAME schedulerHeuristicTest COMMAND schedulerHeuristicTest)
