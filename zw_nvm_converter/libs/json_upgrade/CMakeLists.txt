add_library(json_upgrade "json_upgrade.c")
target_link_libraries(json_upgrade json-c)

target_include_directories(json_upgrade PUBLIC
                            ${json-c_SOURCE_DIR}/json-c
                            ${CMAKE_CURRENT_SOURCE_DIR}
                          )



