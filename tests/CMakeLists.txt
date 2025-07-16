include_directories( ${CMAKE_SOURCE_DIR}/tests 
                     ${cmock_SOURCE_DIR}/src
                     ${cmock_SOURCE_DIR}/vendor/unity/src
)
add_subdirectory(controllerlib)
add_subdirectory(libs)

