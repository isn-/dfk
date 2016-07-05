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

set(DFK_BUILD_UNIT_TESTS TRUE CACHE BOOL "Build unit tests.")
set(DFK_BUILD_AUTO_TESTS TRUE CACHE BOOL "Build auto tests.")
set(DFK_BUILD_SAMPLES TRUE CACHE BOOL "Build code samples.")

set(DFK_PAGE_SIZE ${page_size} CACHE STRING "Memory page size of target platform.")
set(DFK_TCP_BACKLOG 128 CACHE STRING "Default TCP backlog size")
set(DFK_COROUTINE ASM CACHE STRING "Coroutine implementation, options are: ASM.")
set(DFK_NAMED_COROUTINES TRUE CACHE BOOL "Enable user-provided names for coroutines.")
set(DFK_COROUTINE_NAME_LENGTH 32 CACHE STRING "Maximum size of coroutine name, including zero termination byte.")
set(DFK_STACK FIXED CACHE STRING "Stack growth strategy, options are: FIXED, SEGMENTED, COPY, SWAP")
set(DFK_STACK_SIZE 1048576 CACHE STRING "Default stack size, in bytes.")
set(DFK_STACK_ALIGNMENT 16 CACHE STRING "Stack alignment, in bytes.")
set(DFK_EVENT_LOOP LIBUV CACHE STRING "Event loop implementation, options are: LIBUV.")
set(DFK_DEBUG FALSE CACHE BOOL "Emit debug log messages.")
set(DFK_THREADS FALSE CACHE BOOL "Enable multithreading support.")
set(DFK_COVERAGE FALSE CACHE BOOL "Collect gcov coverage statistics.")
set(DFK_VALGRIND FALSE CACHE BOOL "In addition to regular mode, run unit tests under valgrind.")
set(DFK_HTTP_HEADERS_BUFFER 10240 CACHE STRING "Buffer for HTTP headers, stack-allocated.")
set(DFK_IGNORE_SIGPIPE TRUE CACHE BOOL "Ignore SIGPIPE when entering dfk loop.")
set(DFK_ARENA_SEGMENT_SIZE 40960 CACHE STRING "Size of arena's segment, in bytes.")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

check_include_files(assert.h DFK_HAVE_ASSERT_H)
check_include_files(stddef.h DFK_HAVE_STDDEF_H)
check_include_files(stdint.h DFK_HAVE_STDINT_H)
check_include_files(stdlib.h DFK_HAVE_STDLIB_H)
check_include_files(string.h DFK_HAVE_STRING_H)
check_include_files(stdio.h DFK_HAVE_STDIO_H)
check_include_files(stdarg.h DFK_HAVE_STDARG_H)
check_include_files(signal.h DFK_HAVE_SIGNAL_H)
check_include_files(unistd.h DFK_HAVE_UNISTD_H)
check_include_files(pthread.h DFK_HAVE_PTHREAD_H)
check_include_files(sys/socket.h DFK_HAVE_SYS_SOCKET_H)
check_include_files(sys/types.h DFK_HAVE_SYS_TYPES_H)
check_include_files(netinet/in.h DFK_HAVE_NETINET_IN_H)
check_include_files(arpa/inet.h DFK_HAVE_ARPA_INET_H)
check_include_files(ucontext.h DFK_HAVE_UCONTEXT_H)
check_include_files(setjmp.h DFK_HAVE_SETJMP_H)
check_include_files(time.h DFK_HAVE_TIME_H)
check_include_files(ctype.h DFK_HAVE_CTYPE_H)

set(CMAKE_REQUIRED_INCLUDES signal.h)
check_function_exists(sigaltstack DFK_HAVE_SIGALTSTACK)
set(CMAKE_REQUIRED_INCLUDES time.h)
check_function_exists(nanosleep DFK_HAVE_NANOSLEEP)
set(CMAKE_REQUIRED_INCLUDES stdio.h)
check_function_exists(fflush DFK_HAVE_FFLUSH)

if(${DFK_VALGRIND})
  find_program(VALGRIND valgrind REQUIRED)
  mark_as_advanced(VALGRIND)
endif()

if("${DFK_EVENT_LOOP}" STREQUAL LIBUV)
  find_package(libuv REQUIRED)
  include_directories(${LIBUV_INCLUDE_DIRS})
endif()

find_package(http-parser REQUIRED)
include_directories(${HTTP_PARSER_INCLUDE_DIR})

if(DFK_BUILD_UNIT_TESTS)
  find_package(CURL REQUIRED)
endif()

if(DFK_COVERAGE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(FATAL_ERROR "Code coverage can be computed only for Debug builds\nDisable DFK_ENABLE_COVERAGE or change build type to Debug")
endif()

if(DFK_COVERAGE)
  fw_c_flags("--coverage -O0")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/dfk/config.h.in"
  ${CMAKE_CURRENT_BINARY_DIR}/dfk/config.h @ONLY)

if("${DFK_COROUTINE}" STREQUAL ASM)
  add_definitions(-DCORO_ASM)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(thirdparty)

fw_c_flags("-std=c99")
fw_c_flags("-Werror -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition")

if(NOT APPLE AND "${DFK_EVENT_LOOP}" STREQUAL LIBUV)
  add_definitions(-D_GNU_SOURCE)
endif()

add_library(dfkinternal STATIC
  src/list.c
  src/avltree.c
  src/arena.c)

add_library(dfk
  src/core.c
  src/sync.c
  src/tcp_socket.c
  src/http.c)

target_link_libraries(dfk
  dfkinternal
  coro
  ${LIBUV_LIBRARIES}
  ${HTTP_PARSER_LIBRARIES})

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

add_subdirectory(sandbox)
