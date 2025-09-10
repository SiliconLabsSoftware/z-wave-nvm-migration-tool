# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-License-Identifier: Zlib
#
# This recipe allows to download nvm3lib from zipgateway 
# It can be used by projects which are depending on it
# Feel free to copy this (up to date) file everywhere it is needed

# Set zipgateway repository info
set(REPO_URL "https://github.com/SiliconLabs/zipgateway.git")
set(CHECKOUT_DIR "${CMAKE_BINARY_DIR}/zipgateway")
set(NVM3LIB_PATH_RELATIVE "systools/zw_nvm_converter/nvm3lib")
set(NVM3LIB_PATH ${CHECKOUT_DIR}/${NVM3LIB_PATH_RELATIVE})
set(EDIAN_WRAPER_PATH "systools/libs/json_helpers/edian_wrap.h")
# Clone if not already present
if(NOT EXISTS ${CHECKOUT_DIR})
  execute_process(
    COMMAND git clone --no-checkout ${REPO_URL} ${CHECKOUT_DIR}
  )
endif()

# Enable sparse checkout for nvm3lib
execute_process(
  COMMAND git sparse-checkout init --cone
  WORKING_DIRECTORY ${CHECKOUT_DIR}
)

execute_process(
  COMMAND git sparse-checkout set ${NVM3LIB_PATH_RELATIVE} ${EDIAN_WRAPER_PATH}
  WORKING_DIRECTORY ${CHECKOUT_DIR}
)

# Checkout the pinned tag
execute_process(
  COMMAND git checkout zipgateway-7.18.03
  WORKING_DIRECTORY ${CHECKOUT_DIR}
)

add_subdirectory(${NVM3LIB_PATH} ${CMAKE_BINARY_DIR}/nvm3lib)
