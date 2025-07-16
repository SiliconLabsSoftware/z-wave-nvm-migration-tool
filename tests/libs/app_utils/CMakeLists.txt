add_executable(test_user_message test_user_message.c
            ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/app_utils/user_message.c
)
target_include_directories(test_user_message PRIVATE
    ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/app_utils
)
target_link_libraries(test_user_message unity
)
add_test(NAME user_message COMMAND test_user_message)


add_executable(test_base64 test_base64.c
            ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/app_utils/base64.c
)
target_include_directories(test_base64 PRIVATE
    ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/app_utils
)
target_link_libraries(test_base64 unity
)
add_test(NAME base64 COMMAND test_base64)

