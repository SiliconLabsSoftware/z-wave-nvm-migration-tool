add_library(zw_nvm_converter nvmconv_main.c)
add_library (controllerlib_api INTERFACE)

target_include_directories(zw_nvm_converter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(controllerlib_api INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include" ${JSON-C_INCLUDE_DIRS}
)

include(${CMAKE_SOURCE_DIR}/cmake/modules/FindNVM3lib.cmake)
add_subdirectory( controllerlib_7.xx )
add_subdirectory( libs)

target_link_libraries( zw_nvm_converter PUBLIC
                       controllerlib_7xx
                     )
