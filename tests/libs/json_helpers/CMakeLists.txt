add_mock(user_message_mock 
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/app_utils/user_message.h
)

add_executable(test_json_helpers test_json_helpers.c
)

target_include_directories(test_json_helpers PRIVATE
    ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/json_helpers
    PUBLIC
    ${JSON-C_INCLUDE_DIRS}
)
target_link_libraries(test_json_helpers unity 
                      json-c
                      user_message_mock
)
add_test(NAME test_json_helpers COMMAND test_json_helpers)
