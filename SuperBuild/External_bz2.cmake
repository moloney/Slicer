# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Set dependency list
set(bz2_DEPENDENCIES "")

# Include dependent projects if any
SlicerMacroCheckExternalProjectDependency(bz2)
set(proj bz2)

#message(STATUS "${__indent}Adding project ${proj}")

configure_file(
SuperBuild/bz2_make_step.cmake.in
${CMAKE_CURRENT_BINARY_DIR}/bz2_make_step.cmake
@ONLY)

ExternalProject_Add(bz2
  URL http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
  URL_MD5 00b516f4704d4a7cb50a1d97e6e8e15b
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${proj}
  BUILD_IN_SOURCE 1
  BUILD_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/bz2_make_step.cmake
  CONFIGURE_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  )
