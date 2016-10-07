include(CMakeParseArguments)

find_package(Git)

function(fw_c_flags)
  if (NOT CMAKE_C_FLAGS)
    set(CMAKE_C_FLAGS "${ARGN}" PARENT_SCOPE)
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARGN}" PARENT_SCOPE)
  endif()
endfunction()

function(fw_version_from_git out_version out_major out_minor out_patch)
  if(NOT GIT_FOUND)
    message(FATAL_ERROR "Can not get version - 'git' is not installed")
  endif()
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --long
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE retcode
    OUTPUT_VARIABLE version
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT retcode EQUAL 0)
    set(${version} "NOT-FOUND" PARENT_SCOPE)
    return()
  endif()
  string(REGEX REPLACE "v(\\.*)" "\\1" version ${version})
  string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" major ${version})
  string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+.*" "\\1" minor ${version})
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" patch ${version})
  foreach(i version major minor patch)
    set(${out_${i}} ${${i}} PARENT_SCOPE)
  endforeach()
endfunction()

function(fw_version_from_file filename out_version out_major out_minor out_patch)
  file(READ ${filename} content)
  string(STRIP ${content} content)
  set(version ${content})
  string(REGEX REPLACE ".*([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" major ${content})
  string(REGEX REPLACE ".*[0-9]+\\.([0-9]+)\\.[0-9]+.*" "\\1" minor ${content})
  string(REGEX REPLACE ".*[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" patch ${content})
  foreach(i version major minor patch)
    set(${out_${i}} ${${i}} PARENT_SCOPE)
  endforeach()
endfunction()

function(fw_page_size out)
  set(getpagesize "
#include <unistd.h>
#include <stdio.h>

int main()
{
  long sz = sysconf(_SC_PAGESIZE);
  printf(\"%lu\", sz);
  return 0;
}
")
  file(WRITE "${CMAKE_BINARY_DIR}/getpagesize/main.c" "${getpagesize}")
  enable_language(C)
  try_run(
    run_result_unused
    compile_result_unused
    "${CMAKE_BINARY_DIR}/getpagesize"
    "${CMAKE_BINARY_DIR}/getpagesize/main.c"
    RUN_OUTPUT_VARIABLE page_size)
  set(${out} ${page_size} PARENT_SCOPE)
endfunction()

function(fw_stack_size out)
  set(getstacksize "
#include <pthread.h>
#include <stdio.h>

int main()
{
  pthread_attr_t a;
  pthread_attr_init(&a);
  size_t size;
  pthread_attr_getstacksize(&a, &size);
  printf(\"%llu\", (unsigned long long) size);
  return 0;
}
")
  file(WRITE "${CMAKE_BINARY_DIR}/getstacksize/main.c" "${getstacksize}")
  enable_language(C)
  try_run(
    run_result_unused
    compile_result_unused
    "${CMAKE_BINARY_DIR}/getstacksize"
    "${CMAKE_BINARY_DIR}/getstacksize/main.c"
    CMAKE_FLAGS "-DLINK_LIBRARIES=pthread"
    RUN_OUTPUT_VARIABLE stack_size)
  set(${out} ${stack_size} PARENT_SCOPE)
endfunction()

function(fw_guard_size out)
  set(getguardsize "
#include <pthread.h>
#include <stdio.h>

int main()
{
  pthread_attr_t a;
  pthread_attr_init(&a);
  size_t size;
  pthread_attr_getguardsize(&a, &size);
  printf(\"%llu\", (unsigned long long) size);
  return 0;
}
")
  file(WRITE "${CMAKE_BINARY_DIR}/getguardsize/main.c" "${getguardsize}")
  enable_language(C)
  try_run(
    run_result_unused
    compile_result_unused
    "${CMAKE_BINARY_DIR}/getguardsize"
    "${CMAKE_BINARY_DIR}/getguardsize/main.c"
    CMAKE_FLAGS "-DLINK_LIBRARIES=pthread"
    RUN_OUTPUT_VARIABLE guard_size)
  set(${out} ${guard_size} PARENT_SCOPE)
endfunction()
