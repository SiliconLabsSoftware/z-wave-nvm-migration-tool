set(CONTROLLERLIB_7xx_SRC_FILES
  api_wrappers.c
  controller_nvm.c
  nvm3_helpers.c
  zwave/ZW_nodemask.c
)

add_library(controllerlib_7xx ${CONTROLLERLIB_7xx_SRC_FILES})

target_link_libraries(controllerlib_7xx 
                      controllerlib_api 
                      json_helpers 
                      json_upgrade
                      app_utils 
                      bin2hex 
                      nvm3lib 
                      json-c
                      )

target_include_directories(controllerlib_7xx
                            PUBLIC
                              ${JSON-C_INCLUDE_DIRS}
                            PRIVATE
                              ${CMAKE_CURRENT_SOURCE_DIR}
                              ${CMAKE_CURRENT_SOURCE_DIR}/zwave
                              ../include
                          )
