add_library(json_helpers "json_helpers.c")
target_link_libraries(json_helpers app_utils json-c)

target_include_directories(json_helpers PUBLIC
                            ${json-c_SOURCE_DIR}/json-c
                            ${CMAKE_CURRENT_SOURCE_DIR}
                          )

