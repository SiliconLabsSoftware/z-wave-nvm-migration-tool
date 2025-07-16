add_executable(test_ZW_nodemask test_ZW_nodemask.c
            ${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx/zwave/ZW_nodemask.c
)

target_include_directories(test_ZW_nodemask PRIVATE
    ${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx/zwave
)
target_link_libraries(test_ZW_nodemask unity
)

add_test(NAME ZW_nodemask COMMAND test_ZW_nodemask)
