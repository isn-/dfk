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

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(DFK_BUILD_UNIT_TESTS TRUE CACHE BOOL "")
set(DFK_BUILD_AUTO_TESTS TRUE CACHE BOOL "")
set(DFK_BUILD_SAMPLES TRUE CACHE BOOL "")

set(DFK_PAGE_SIZE ${page_size} CACHE STRING "")
set(DFK_DEFAULT_STACK_SIZE 1048576 CACHE STRING "")
set(DFK_DEFAULT_TCP_BACKLOG 128 CACHE STRING "")
set(DFK_COROUTINE_ASM TRUE CACHE BOOL "")
set(DFK_EVENT_LOOP_LIBUV TRUE CACHE BOOL "")
set(DFK_ENABLE_DEBUG TRUE CACHE BOOL "")
set(DFK_ENABLE_COVERAGE FALSE CACHE BOOL "")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

check_include_files(assert.h DFK_HAVE_ASSERT_H)
check_include_files(stddef.h DFK_HAVE_STDDEF_H)
check_include_files(stdint.h DFK_HAVE_STDINT_H)
check_include_files(stdlib.h DFK_HAVE_STDLIB_H)
check_include_files(string.h DFK_HAVE_STRING_H)
check_include_files(stdio.h DFK_HAVE_STDIO_H)
check_include_files(signal.h DFK_HAVE_SIGNAL_H)
check_include_files(unistd.h DFK_HAVE_UNISTD_H)
check_include_files(pthread.h DFK_HAVE_PTHREAD_H)
check_include_files(sys/socket.h DFK_HAVE_SYS_SOCKET_H)
check_include_files(netinet/in.h DFK_HAVE_NETINET_IN_H)
check_include_files(arpa/inet.h DFK_HAVE_ARPA_INET_H)
check_include_files(ucontext.h DFK_HAVE_UCONTEXT_H)
check_include_files(setjmp.h DFK_HAVE_SETJMP_H)

list(APPEND CMAKE_REQUIRED_INCLUDES signal.h)
check_function_exists(sigaltstack DFK_HAVE_SIGALTSTACK)

find_program(VALGRIND valgrind)
mark_as_advanced(VALGRIND)

if(DFK_EVENT_LOOP_LIBUV)
  find_package(LibUV REQUIRED)
  include_directories(${LIBUV_INCLUDE_DIRS})
endif()

if(DFK_ENABLE_COVERAGE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(FATAL_ERROR "Code coverage can be computed only for Debug builds\nDisable DFK_ENABLE_COVERAGE or change build type to Debug")
endif()

if(DFK_ENABLE_COVERAGE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -O0")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/dfk/config.h.in"
  ${CMAKE_CURRENT_BINARY_DIR}/dfk/config.h @ONLY)

if(DFK_COROUTINE_ASM)
  add_definitions(-DCORO_ASM)
endif()

add_subdirectory(thirdparty)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition")

add_library(dfk
  src/core.c
  src/context.c
  src/buf.c
  src/coro.c
  src/event_loop.c
  src/tcp_socket.c)

target_link_libraries(dfk
  coro
  ${LIBUV_LIBRARIES})

enable_testing()

if(DFK_BUILD_UNIT_TESTS)
  add_subdirectory(ut)
endif()

if(DFK_BUILD_AUTO_TESTS)
  add_subdirectory(at)
endif()

if(DFK_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()
