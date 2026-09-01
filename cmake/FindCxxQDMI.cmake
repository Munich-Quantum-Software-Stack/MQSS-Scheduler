# This file has been superseded by FindQDMIDeviceExample.cmake.
# It is kept here only to avoid breaking any stale CMakeCache entries.
# Use find_package(QDMIDeviceExample REQUIRED) instead.
message(
  WARNING
    "FindCxxQDMI.cmake is deprecated — use find_package(QDMIDeviceExample REQUIRED) instead."
)
include("${CMAKE_CURRENT_LIST_DIR}/FindQDMIDeviceExample.cmake")
set(CXXQDMI_FOUND ${QDMI_DEVICE_EXAMPLE_FOUND})
set(CXXQDMI_INCLUDE_DIR ${QDMI_DEVICE_EXAMPLE_INCLUDE_DIR})
set(CXXQDMI_INCLUDE_DIRS ${QDMI_DEVICE_EXAMPLE_INCLUDE_DIRS})
set(CXXQDMI_LIBRARIES ${QDMI_DEVICE_EXAMPLE_LIBRARIES})
