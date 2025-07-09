cmake_minimum_required(VERSION 3.7)

project(
  z-wave-nvm-migration-tool
  VERSION 1.0.0
	LANGUAGES C CXX ASM
)

find_package(json-c QUIET)
if(NOT json-c_FOUND)
  message(STATUS "json-c not found, fetching via FetchContent")
  include(cmake/modules/FindJsonC.cmake)
  set(JSON-C_INCLUDE_DIRS ${json-c_SOURCE_DIR})
else()
  find_path(JSON-C_INCLUDE_DIRS NAMES json-c PATH_SUFFIXES include)
  set(JSON-C_INCLUDE_DIRS "${JSON-C_INCLUDE_DIRS}/json-c")
  message(STATUS "json-c include directory: ${JSON-C_INCLUDE_DIRS}")
endif()
# Added option for debugging
option(ENABLE_DEBUG "Debug Mode is ON/OFF" OFF)

if(ENABLE_DEBUG)
  message("Debug mode is enabled")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_DEBUG_POSTFIX d)

add_subdirectory(zw_nvm_converter)

add_executable( z-wave-nvm-migration-tool zwave_nvm_tool.c )
set_target_properties(z-wave-nvm-migration-tool PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries( z-wave-nvm-migration-tool PRIVATE
                       zw_nvm_converter
                     )
install(TARGETS z-wave-nvm-migration-tool RUNTIME DESTINATION bin)
install(FILES
"${CMAKE_SOURCE_DIR}/README.md"
"${CMAKE_SOURCE_DIR}/LICENSE"
TYPE DOC
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Silicon Labs")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libjson-c5 | libjson-c4 | libjson-c3 | libjson-c2")

include(CPack)