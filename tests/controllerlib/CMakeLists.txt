set(NVM3LIB_PATH "${CMAKE_BINARY_DIR}/zipgateway/systools/zw_nvm_converter/nvm3lib")
add_mock(nvm3_mock 
  ${CMAKE_SOURCE_DIR}/tests/mock/nvm3_stubs.h
)
target_include_directories(nvm3_mock
  PUBLIC
  ${NVM3LIB_PATH}/platform/emdrv/nvm3/inc/
  ${NVM3LIB_PATH}/platform/emdrv/nvm3/config/
  ${NVM3LIB_PATH}/platform/emdrv/common/inc/
	${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx/zwave
)
# Build nvm3_mock with HOST_BUILD and set FLASH_PAGE_SIZE and NVM3_DEFAULT_CACHE_SIZE for host environment to be applied with nvm3_hal
target_compile_definitions(nvm3_mock PUBLIC NVM3_HOST_BUILD FLASH_PAGE_SIZE=2048 NVM3_DEFAULT_CACHE_SIZE=400)
add_mock(json_helpers_mock
  ${CMAKE_SOURCE_DIR}/tests/mock/json_helpers.h
)
target_include_directories(json_helpers_mock
  PRIVATE
  ${JSON-C_INCLUDE_DIRS}
  ${json-c_BINARY_DIR}
)
add_mock(nvm3_helpers_mock
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx/nvm3_helpers.h
)
target_include_directories(nvm3_helpers_mock
  PUBLIC
  ${NVM3LIB_PATH}/platform/emdrv/nvm3/inc/
  ${NVM3LIB_PATH}/platform/emdrv/nvm3/config/
  ${NVM3LIB_PATH}/platform/emdrv/common/inc/
	${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx/zwave
)
target_compile_definitions(nvm3_helpers_mock PUBLIC NVM3_HOST_BUILD FLASH_PAGE_SIZE=2048 NVM3_DEFAULT_CACHE_SIZE=400)
add_mock(base64_mock
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/app_utils/base64.h
)
add_mock(ZW_nodemask_api_mock 
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx/zwave/ZW_nodemask_api.h
)

add_executable(test_nvm3_helpers test_nvm3_helpers.c
)
target_include_directories(test_nvm3_helpers PRIVATE 
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx
)
target_link_libraries(test_nvm3_helpers 
            unity
            nvm3_mock
					  user_message_mock
)
add_test(NAME test_nvm3_helpers COMMAND test_nvm3_helpers)

add_executable(test_controller_nvm test_controller_nvm.c
)
target_include_directories(test_controller_nvm PRIVATE
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx/zwave
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/controllerlib_7.xx
  ${CMAKE_SOURCE_DIR}/zw_nvm_converter/libs/json_helpers
  PUBLIC
  ${JSON-C_INCLUDE_DIRS}
  unity/src
)
target_link_libraries(test_controller_nvm 
            unity
            nvm3lib
            json-c
            json_helpers_mock
            nvm3_helpers_mock
            user_message_mock
            ZW_nodemask_api_mock
            base64_mock
)

add_test(NAME controller_nvm COMMAND test_controller_nvm)

add_subdirectory(zwave)