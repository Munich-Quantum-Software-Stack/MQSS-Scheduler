find_package(GTest REQUIRED)


include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

include(GoogleTest)
add_test(NAME SchedulerTest COMMAND "${TODO}" )
