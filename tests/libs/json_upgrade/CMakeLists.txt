add_executable(test_json_upgrade test_json_upgrade.c
)

target_include_directories(test_json_upgrade PRIVATE
    ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/json_upgrade
    PUBLIC
    ${JSON-C_INCLUDE_DIRS}
)
target_link_libraries(test_json_upgrade unity 
                      json-c
                      )
add_test(NAME json_upgrade COMMAND test_json_upgrade)
