# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-License-Identifier: Zlib
#
# This recipe allows to download json-c
# It can be used by projects which are depending on it
# Feel free to copy this (up to date) file everywhere it is needed

include(FetchContent)

if(NOT DEFINED JSONC_GIT_REPOSITORY)
  if(DEFINED ENV{JSONC_GIT_REPOSITORY})
    set(JSONC_GIT_REPOSITORY $ENV{JSONC_GIT_REPOSITORY})
  endif()
endif()
if("${JSONC_GIT_REPOSITORY}" STREQUAL "")
  set(JSONC_GIT_REPOSITORY "https://github.com/json-c/json-c")
endif()

if(NOT DEFINED JSONC_GIT_TAG)
  if(DEFINED ENV{JSONC_GIT_TAG})
    set(JSONC_GIT_TAG $ENV{JSONC_GIT_TAG})
  else()
    set(JSONC_GIT_TAG "json-c-0.16-20220414")
  endif()
endif()
  
FetchContent_Declare(
  json-c
  GIT_REPOSITORY ${JSONC_GIT_REPOSITORY}
  GIT_TAG        ${JSONC_GIT_TAG}
  GIT_SUBMODULES_RECURSE True
  GIT_SHALLOW 1
)

set(BUILD_TESTING OFF CACHE BOOL "Disable building tests")
set(BUILD_SHARED_LIBS ON CACHE BOOL "Enable building shared libraries")

message(STATUS "${CMAKE_PROJECT_NAME}: Depends: ${JSONC_GIT_REPOSITORY}#${JSONC_GIT_TAG}")
string(REGEX MATCH ".*/?main/?.*" JSONC_UNSTABLE_GIT_TAG "${JSONC_GIT_TAG}")
if(JSONC_GIT_TAG STREQUAL "" OR JSONC_UNSTABLE_GIT_TAG)
  message(WARNING "${CMAKE_PROJECT_NAME}: Declare JSONC_GIT_TAG to stable version not: ${JSONC_UNSTABLE_GIT_TAG}")
endif()

set(FETCHCONTENT_QUIET FALSE)
FetchContent_MakeAvailable(json-c)

message(STATUS "json-c Sources: ${json-c_SOURCE_DIR}")
message(STATUS "json-c Binaries: ${json-c_BINARY_DIR}")