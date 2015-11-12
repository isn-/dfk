cmake_minimum_required(VERSION 2.8)
project(dfk C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(fw)
include(CheckIncludeFiles)
include(CheckSymbolExists)

fw_version_from_git(DFK_VERSION DFK_VERSION_MAJOR DFK_VERSION_MINOR DFK_VERSION_PATCH)
fw_page_size(page_size)

set(DFK_ENABLE_UNIT_TESTS TRUE CACHE BOOL "")
set(DFK_ENABLE_AUTO_TESTS TRUE CACHE BOOL "")
set(DFK_ENABLE_BUFFER_MAGIC TRUE CACHE BOOL "")

set(DFK_PAGE_SIZE ${page_size} CACHE STRING "")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition")

check_include_files(assert.h HAVE_ASSERT_H)
check_include_files(stddef.h HAVE_STDDEF_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(stdio.h HAVE_STDIO_H)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/dfk/config.h.in"
  ${CMAKE_CURRENT_BINARY_DIR}/dfk/config.h @ONLY)

add_library(dfk
  src/context.c
  src/bufman_hk.c
  src/bufman.c)

enable_testing()

if(DFK_ENABLE_UNIT_TESTS)
  add_subdirectory(ut)
endif()

if(DFK_ENABLE_AUTO_TESTS)
  add_subdirectory(at)
endif()
