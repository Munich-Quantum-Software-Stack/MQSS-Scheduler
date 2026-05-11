# This file has been superseded by FindFakeQDMI_Dev.cmake.
# It is kept here only to avoid breaking any stale CMakeCache entries.
# Use find_package(FakeQDMI_Dev REQUIRED) instead.
message(WARNING "FindCxxQDMI.cmake is deprecated — use find_package(FakeQDMI_Dev REQUIRED) instead.")
include("${CMAKE_CURRENT_LIST_DIR}/FindFakeQDMI_Dev.cmake")
set(CXXQDMI_FOUND        ${FAKEQDMI_DEV_FOUND})
set(CXXQDMI_INCLUDE_DIR  ${FAKEQDMI_DEV_INCLUDE_DIR})
set(CXXQDMI_INCLUDE_DIRS ${FAKEQDMI_DEV_INCLUDE_DIRS})
set(CXXQDMI_LIBRARIES    ${FAKEQDMI_DEV_LIBRARIES})
