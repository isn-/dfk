cmake_minimum_required(VERSION 2.8)
project(dfk C)

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  message(FATAL_ERROR "\nIn-source builds are not allowed.\nTry 'mkdir build && cd build && cmake ..'")
endif()

set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpm_packages)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(fw)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)

fw_version_from_git(DFK_VERSION DFK_VERSION_MAJOR DFK_VERSION_MINOR DFK_VERSION_PATCH)
fw_page_size(page_size)

set(DFK_ENABLE_UNIT_TESTS TRUE CACHE BOOL "")
set(DFK_ENABLE_AUTO_TESTS TRUE CACHE BOOL "")
set(DFK_ENABLE_MAGIC TRUE CACHE BOOL "")
set(DFK_ENABLE_CLEANUP TRUE CACHE BOOL "")
set(DFK_BUFFER_MANAGER_HOUSEKEEPING_SIZE 4096 CACHE STRING "")
set(DFK_DEFAULT_STACK_SIZE 10240 CACHE STRING "")
set(DFK_COROUTINE_ASM TRUE CACHE STRING "")

set(DFK_PAGE_SIZE ${page_size} CACHE STRING "")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

check_include_files(assert.h HAVE_ASSERT_H)
check_include_files(stddef.h HAVE_STDDEF_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(stdio.h HAVE_STDIO_H)
check_include_files(signal.h HAVE_SIGNAL_H)
check_include_files(ucontext.h HAVE_UCONTEXT_H)
check_include_files(setjmp.h HAVE_SETJMP_H)

list(APPEND CMAKE_REQUIRED_INCLUDES signal.h)
check_function_exists(sigaltstack HAVE_SIGALTSTACK)

find_program(VALGRIND valgrind)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/dfk/config.h.in"
  ${CMAKE_CURRENT_BINARY_DIR}/dfk/config.h @ONLY)

if(DFK_COROUTINE_ASM)
  add_definitions(-DCORO_ASM)
endif()
add_subdirectory(thirdparty)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition")

add_library(dfk
  src/context.c
  src/bufman_hk.c
  src/bufman.c
  src/coro.c)

target_link_libraries(dfk
  coro)

enable_testing()

if(DFK_ENABLE_UNIT_TESTS)
  add_subdirectory(ut)
endif()

if(DFK_ENABLE_AUTO_TESTS)
  add_subdirectory(at)
endif()
