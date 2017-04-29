cmake_minimum_required(VERSION 2.8)
project(dfk C CXX)

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  message(FATAL_ERROR "\nIn-source builds are not allowed.\nTry 'mkdir build && cd build && cmake ..'")
endif()

set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpm_packages)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(fw)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)

fw_version_from_file(version DFK_VERSION DFK_VERSION_MAJOR DFK_VERSION_MINOR DFK_VERSION_PATCH)
fw_version_from_git(DFK_VERSION DFK_VERSION_MAJOR DFK_VERSION_MINOR DFK_VERSION_PATCH)
fw_page_size(page_size)
fw_stack_size(stack_size)
fw_guard_size(guard_size)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(DFK_BUILD_UNIT_TESTS TRUE CACHE BOOL "Build unit tests.")
set(DFK_BUILD_AUTO_TESTS TRUE CACHE BOOL "Build auto tests.")
set(DFK_BUILD_SAMPLES FALSE CACHE BOOL "Build code samples.")
set(DFK_BUILD_CPP_BINDINGS FALSE CACHE BOOL "Build c++ bindings.")

set(DFK_MAINTAINER_MODE FALSE CACHE BOOL "Enable binary package maintainer mode. Designed for deb/rpm packages maintainers. Disables -Werror and checks that neither of release-unfriendly options are set.")

set(DFK_PAGE_SIZE ${page_size} CACHE STRING "Memory page size of target platform.")
set(DFK_TCP_BACKLOG 128 CACHE STRING "Default TCP backlog size.")
set(DFK_COROUTINE ASM CACHE STRING "Coroutine implementation, options are: ASM.")
set(DFK_NAMED_COROUTINES TRUE CACHE BOOL "Enable user-provided names for coroutines.")
set(DFK_COROUTINE_NAME_LENGTH 32 CACHE STRING "Maximum size of coroutine name, including zero termination byte.")
set(DFK_STACK FIXED CACHE STRING "Stack growth strategy, options are: FIXED.")
set(DFK_STACK_SIZE ${stack_size} CACHE STRING "Default stack size, in bytes.")
set(DFK_STACK_ALIGNMENT 16 CACHE STRING "Stack alignment, in bytes.")
set(DFK_STACK_GUARD_SIZE ${guard_size} CACHE STRING "Emit N guard bytes to protect against stack overflow.")
set(DFK_DEBUG FALSE CACHE BOOL "Emit debug log messages.")
set(DFK_MOCKS TRUE CACHE BOOL "Enable object mocking for unit testing. If set to OFF, some tests will be unavailable.")
set(DFK_THREADS FALSE CACHE BOOL "Enable multithreading support.")
set(DFK_COVERAGE FALSE CACHE BOOL "Collect gcov coverage statistics.")
set(DFK_VALGRIND FALSE CACHE BOOL "In addition to regular mode, run unit tests under valgrind.")
set(DFK_THREAD_SANITIZER FALSE CACHE BOOL "Enable Thread Sanitizer.")
set(DFK_MEMORY_SANITIZER FALSE CACHE BOOL "Enable Memory Sanitizer.")
set(DFK_HTTP_KEEPALIVE_REQUESTS 100 CACHE STRING "Maximum number of requests for a single keepalive connection. Negative values mean no limit.")
set(DFK_HTTP_HEADERS_BUFFER_SIZE 16384 CACHE STRING "Size of the buffer allocated for HTTP header parsing.")
set(DFK_HTTP_HEADERS_BUFFER_COUNT 8 CACHE STRING "Maximum number of buffers of size DFK_HTTP_HEADERS_BUFFER_SIZE consumed by HTTP request parser.")
set(DFK_HTTP_HEADER_MAX_SIZE 8192 CACHE STRING "Limit of the individual HTTP header line - url, \"field: value\".")
set(DFK_HTTP_PIPELINING TRUE CACHE STRING "Enable HTTP requests pipelining.")
set(DFK_IGNORE_SIGPIPE TRUE CACHE BOOL "Ignore SIGPIPE when entering dfk loop.")
set(DFK_LIST_CONSTANT_TIME_SIZE TRUE CACHE BOOL "Store size of the dfk_list_t")
set(DFK_LIST_MEMORY_OPTIMIZED FALSE CACHE BOOL
  "Enable XOR'ed pointers optimization for doubly linked list.")
set(DFK_AVLTREE_CONSTANT_TIME_SIZE TRUE CACHE BOOL "Store size of the dfk_avltree_t")
set(DFK_ARENA_SEGMENT_SIZE 40960 CACHE STRING "Size of arena's segment, in bytes.")
set(DFK_SPONGE_INITIAL_SIZE 4096 CACHE STRING "Size of sponge data structure initial size, in bytes.")
set(DFK_URLENCODING_HINT_HEURISTICS TRUE CACHE BOOL "Enable heuristics for dfk_urlencode_hint, dfk_urldecode_hint.")
set(DFK_FILESERVER TRUE CACHE BOOL "Enable fileserver middleware")
set(DFK_FILESERVER_BUFFER_SIZE 4096 CACHE STRING "Size of disk IO buffer for each connecion")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/include")
if(DFK_BUILD_AUTO_TESTS)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/c++/include")
endif()

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
check_include_files(sys/mman.h DFK_HAVE_SYS_MMAN_H)
check_include_files(sys/uio.h DFK_HAVE_SYS_UIO_H)
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
set(CMAKE_REQUIRED_INCLUDES sys/mman.h sys/types.h)
check_function_exists(mprotect DFK_HAVE_MPROTECT)

# Searching for packages

if(DFK_VALGRIND)
  find_program(VALGRIND valgrind REQUIRED)
  mark_as_advanced(VALGRIND)
endif()

find_package(http-parser REQUIRED)
include_directories(${HTTP_PARSER_INCLUDE_DIR})

if(DFK_BUILD_UNIT_TESTS)
  find_package(CURL REQUIRED)
  include_directories(${CURL_INCLUDE_DIR})
  find_package(Threads REQUIRED)
endif()

# User-provided compile-time options verification goes below.
# If desired options lead to unsatisfiable configuration, e.g.
# required syscall is missing, report it here via message(FATAL_ERROR ...)

if(DFK_COVERAGE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(FATAL_ERROR "Code coverage can be computed only for Debug builds.\nDisable DFK_ENABLE_COVERAGE or change build type to Debug.")
endif()

if(DFK_STACK_GUARD_SIZE AND (NOT DFK_HAVE_MPROTECT OR NOT DFK_HAVE_SYS_MMAN_H))
  message(FATAL_ERROR "Can not emit guard pages without mprotect function from <sys/mman.h> header.")
endif()

if(DFK_MAINTAINER_MODE)
  set(disallowed_options
    DFK_COVERAGE
    DFK_THREAD_SANITIZER
    DFK_MEMORY_SANITIZER
    DFK_VALGRIND)
  foreach(opt ${disallowed_options})
    if(${${opt}})
      message(FATAL_ERROR "${opt} is not allowed in maintainer mode.\nSet ${opt} to OFF, or disable DFK_MAINTAINER_MODE.")
    endif()
  endforeach()
  set(not_recommended_options
    DFK_MOCKS
    DFK_DEBUG)
  foreach(opt ${not_recommended_options})
    if(${${opt}})
      message(WARNING "${opt} is not recommended in maintainer mode.\nSet ${opt} to OFF, or disable DFK_MAINTAINER_MODE to suppress that warning.")
    endif()
  endforeach()
endif()

# Add compiler flags to satisfy user-provided configuration options.

if(DFK_THREAD_SANITIZER)
  fw_c_cxx_flags("-fsanitize=thread")
endif()

if(DFK_MEMORY_SANITIZER)
  fw_c_cxx_flags("-fsanitize=memory")
  fw_c_cxx_flags("-fno-omit-frame-pointer -fno-optimize-sibling-calls")
endif()

if(DFK_COVERAGE)
  fw_c_cxx_flags("--coverage -O0")
endif()

if(NOT DFK_MAINTAINER_MODE)
  fw_c_cxx_flags("-Werror")
endif()

if("${DFK_COROUTINE}" STREQUAL ASM)
  # CORO_ASM variable is needed for dfk/config.h
  set(CORO_ASM true)
  add_definitions(-DCORO_ASM)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/dfk/config.h.in"
  ${CMAKE_CURRENT_BINARY_DIR}/dfk/config.h @ONLY)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/thirdparty" thirdparty
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/dfk")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(thirdparty)

# Enable C11 for dfk code
if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_C_COMPILER_VERSION VERSION_LESS 4.7)
  fw_c_flags("-std=c1x")
else()
  fw_c_flags("-std=c11")
endif()

# Enable hardcore warnings mode.
fw_c_flags("-Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition")

set(dfk_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/src/list.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/avltree.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/context.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/error.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/strtoll.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/misc.c"
  #  "${CMAKE_CURRENT_SOURCE_DIR}/src/arena.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/strmap.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/sponge.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/core.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/sync.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/tcp_socket.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/urlencoding.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/http/constants.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/http/request.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/http/response.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/http/protocol.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/http/server.c"
)

#if(DFK_FILESERVER)
#  list(APPEND dfk_sources
#    "${CMAKE_CURRENT_SOURCE_DIR}/src/middleware/fileserver.c")
#endif()

add_library(dfk ${dfk_sources})

target_link_libraries(dfk
  coro
  ${HTTP_PARSER_LIBRARIES})

set_target_properties(dfk
  PROPERTIES
  VERSION ${DFK_VERSION_MAJOR}.${DFK_VERSION_MINOR}.${DFK_VERSION_PATCH}
  SOVERSION ${DFK_VERSION_MAJOR}.${DFK_VERSION_MINOR})

install(TARGETS dfk DESTINATION lib)

fw_exports(dfk dfk.exports)

install(DIRECTORY include/dfk DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES
  include/dfk.h
  DESTINATION include)
install(FILES
  ${CMAKE_BINARY_DIR}/dfk/config.h
  DESTINATION include/dfk)

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

if(DFK_BUILD_CPP_BINDINGS)
  add_subdirectory(c++)
endif()

# A directory for experimenting, no file from this directory except empty
# CMakeLists.txt should be committed into the repository
add_subdirectory(sandbox)

configure_file(dfk.pc.in "${CMAKE_CURRENT_BINARY_DIR}/dfk.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dfk.pc" DESTINATION lib/pkgconfig)
