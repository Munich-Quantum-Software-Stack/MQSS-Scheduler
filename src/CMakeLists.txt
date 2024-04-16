
message(DEBUG "Building Scheduler")
# Add the include paths
include_directories(
  "${PROJECT_SOURCE_DIR}/include"
  "${CMAKE_INSTALL_PREFIX}/include"
  ${LLVM_INCLUDE_DIRS})

set(LIB_NAME "qresourcemanager_d" CACHE STRING "" FORCE)

# Build the executable
set(SOURCES *.cpp)
set(PUBLIC_SCHEDULER_HEADERS "")

# Build the executable
add_library(${LIB_NAME} ${SOURCES})

set_target_properties(${LIB_NAME} PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}
  PUBLIC_HEADER "${PUBLIC_SCHEDULER_HEADERS}"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
  BUILD_WITH_INSTALL_RPATH TRUE
)

# Link the executable with the required libraries
target_link_libraries(${LIB_NAME} PRIVATE
  LLVM
  Threads::Threads
)

# Include header files
target_include_directories(${LIB_NAME}
  PRIVATE /usr/local/include # RabbitMQ
  ${CMAKE_INSTALL_PREFIX}/include
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Install the Quantum Scheduler
install(TARGETS ${LIB_NAME}
  EXPORT QuantumSchedulerTargets
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
