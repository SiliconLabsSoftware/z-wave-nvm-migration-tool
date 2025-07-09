add_library(json_helpers "json_helpers.c")
target_link_libraries(json_helpers app_utils json-c)

target_include_directories(json_helpers PUBLIC
                            ${JSON-C_INCLUDE_DIRS}
                            ${CMAKE_CURRENT_SOURCE_DIR}
                          )

