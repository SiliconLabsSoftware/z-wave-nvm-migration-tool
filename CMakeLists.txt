cmake_minimum_required(VERSION 3.7)

project(
  z-wave-nvm-migration-tool
  VERSION 1.0.1
	LANGUAGES C CXX ASM
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/zwave_nvm_tool_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/zwave_nvm_tool_version.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

option(COVERAGE "Enable coverage reporting" OFF)
if(COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Building with code coverage")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

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

if (NOT DEFINED SKIP_TESTING)
  include(cmake/modules/FindCMock.cmake)
# compile the unity version bundled along with cmock sources.
 add_library(unity ${cmock_SOURCE_DIR}/vendor/unity/src/unity.c)
 target_include_directories(unity PUBLIC ${cmock_SOURCE_DIR}/vendor/unity/src)
 target_compile_options(unity PRIVATE "-fPIC")

#  Build the cmock library and link the above compiled unity with the cmock library
 add_library(cmock STATIC ${cmock_SOURCE_DIR}/src/cmock.c)
 target_include_directories(cmock PUBLIC ${cmock_SOURCE_DIR})
 target_link_libraries(cmock unity)
 target_compile_options(cmock PRIVATE "-fPIC")
# and the add_mock functions to make it available to the individual modules 
 include(cmake/include/add_mock.cmake)
  include(CTest)
  message(STATUS "Enable testing for zwave_nvm_tool")
  enable_testing()
  # find_program( MEMORYCHECK_COMMAND valgrind )
  add_subdirectory(tests)
endif()

add_subdirectory(zw_nvm_converter)

add_executable( z-wave-nvm-migration-tool zwave_nvm_tool.c )
set_target_properties(z-wave-nvm-migration-tool PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries( z-wave-nvm-migration-tool PRIVATE
                       zw_nvm_converter
                     )
install(TARGETS z-wave-nvm-migration-tool RUNTIME DESTINATION bin)
install(FILES
  "${CMAKE_SOURCE_DIR}/LICENSE"
  "${CMAKE_SOURCE_DIR}/README.md"
  "${CMAKE_SOURCE_DIR}/CHANGES.md"
  "${CMAKE_SOURCE_DIR}/zw_nvm_converter/schema-json/zwave_data_description_scheme.json"
  TYPE DOC
)

set(CPACK_PACKAGE_VENDOR "Silicon Labs")
set(CPACK_PACKAGE_CONTACT "https://github.com/SiliconLabsSoftware/z-wave-nvm-migration-tool")
if(NOT CPACK_GENERATOR)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libjson-c5 | libjson-c4 | libjson-c3 | libjson-c2")
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
endif()

message(${CPACK_PACKAGE_FILE_NAME} "")

include(CPack)
