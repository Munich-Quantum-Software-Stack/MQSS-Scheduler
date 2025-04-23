find_package(submitter REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include/setup)

add_subdirectory(setup)
add_subdirectory(round_robin)
# add_subdirectory(backfilling)