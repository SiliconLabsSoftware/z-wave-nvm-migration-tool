cmake_minimum_required(VERSION 3.7)

project(
  zwave_nvm_tool
  VERSION 1.0.0
	LANGUAGES C CXX ASM
)

include(cmake/modules/FindJsonC.cmake)
# Added option for debugging
option(ENABLE_DEBUG "Debug Mode is ON/OFF" OFF)

if(ENABLE_DEBUG)
  message("Debug mode is enabled")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_DEBUG_POSTFIX d)

add_subdirectory(zw_nvm_converter)

add_executable( zwave_nvm_tool zwave_nvm_tool.c )
set_target_properties(zwave_nvm_tool PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries( zwave_nvm_tool PRIVATE
                       zw_nvm_converter
                     )
install(TARGETS zwave_nvm_tool RUNTIME DESTINATION bin)
install(FILES
"${CMAKE_SOURCE_DIR}/README.md"
"${CMAKE_SOURCE_DIR}/LICENSE"
TYPE DOC
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Silicon Labs")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libjson-c2 | libjson-c3 | libjson-c4")


include(CPack)