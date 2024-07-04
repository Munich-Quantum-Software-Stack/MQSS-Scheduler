find_package(fomac REQUIRED)
find_package(qdmi REQUIRED)
find_package(qinfo REQUIRED)
#find_package(submitter REQUIRED)

set(src_name "milq")

# Set RPATH for the shared libraries in this directory
set(CMAKE_INSTALL_RPATH "${INSTALL_PATH}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

file(GLOB src_files ${src_file} *.cpp)
target_sources(${src_name} PUBLIC ${src_files})
target_link_libraries(${src_name} PRIVATE
  QirPassRunner qdmi qinfo fomac)

set_target_properties(
  ${src_name}
  PROPERTIES
  INSTALL_RPATH
  "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH_USE_LINK_PATH TRUE)

# Set include directories for the library
target_include_directories(${src_name} PRIVATE ${CMAKE_INSTALL_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include/milq)

# Install the shared library
install(TARGETS ${src_name} DESTINATION "${INSTALL_PATH}/lib")
