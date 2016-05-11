/**
 * @file dfk/core.h
 * Core definitions
 *
 * @copyright
 * Copyright (c) 2015, 2016, Stanislav Ivochkin. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <stddef.h>
#include <uv.h>
#include <libcoro/coro.h>
#include <dfk/config.h>

/**
 * @file dfk/tcp_socket.h
 * TCP socket object and related functions
 *
 * @defgroup core core
 * @{
 */


/**
 * Logging stream
 *
 * Also could be considered as logging level.
 */
typedef enum dfk_log_e {
  /**
   * Error messages
   *
   * An inevitable error has occured. Manual intervention is needed
   * to recover.
   */
  dfk_log_error = 0,
  /**
   * Warning message
   *
   * Normal execution path has been violated, although library knows
   * how to recover from it. E.g. TCP connection was unexpectedly
   * closed by remote peer.
   *
   * Presence of warning messages in the log may indicate that some
   * extra configuration of external systems, or inside dfk library
   * itself is required.
   */
  dfk_log_warning = 1,
  /**
   * Informational message
   *
   * An event that might be useful for system monitoring has been occured.
   */
  dfk_log_info = 2,
  /**
   * Debug messages
   *
   * A very verbose logging level. Disabled by default, can be enabled
   * by setting DFK_DEBUG compile-time flag.
   */
  dfk_log_debug = 3
} dfk_log_e;


/**
 * Error codes returned by dfk functions
 */
typedef enum dfk_error_e {
  /**
   * No error
   */
  dfk_err_ok = 0,

  /**
   * Memory allocation function returned NULL
   */
  dfk_err_nomem,

  /**
   * Object not found
   */
  dfk_err_notfound,

  /**
   * Bad argument
   *
   * Passing NULL pointer to the function that expects non-NULL
   * will yield this error.
   */
  dfk_err_badarg,

  /**
   * System error, see dfk_t.sys_errno
   */
  dfk_err_sys,

  /**
   * The operation is already in progress
   */
  dfk_err_inprog,

  /**
   * Unexpected behaviour, e.g. unreachable code executed
   *
   * Please submit a bug report if API returns this error code.
   * https://github.com/isn-/dfk/issues/new
   */
  dfk_err_panic,

  _dfk_err_total
} dfk_error_e;


typedef struct dfk_iovec_t {
  char* data;
  size_t size;
} dfk_iovec_t;


/**
 * dfk library context
 */
typedef struct dfk_t {
  struct {
    struct dfk_coro_t* exechead;
    struct dfk_coro_t* termhead;
    struct dfk_coro_t* scheduler;
    struct dfk_coro_t* eventloop;
    uv_loop_t* uvloop;
  } _;

  void* userdata;

  void* (*malloc) (void*, size_t);
  void (*free) (void*, void*);
  void* (*realloc)(void*, void*, size_t);

  void (*log)(void*, int, const char*);

  size_t default_stack_size;

  int sys_errno;
  int dfk_errno;

} dfk_t;


/**
 * Coroutine context
 */
typedef struct dfk_coro_t {
  struct {
    struct coro_context ctx;
    struct dfk_coro_t* next;
    void (*ep)(struct dfk_coro_t*, void*);
    void* arg;
#ifdef DFK_NAMED_COROUTINES
    char name[DFK_COROUTINE_NAME_LENGTH];
#endif
#ifdef DFK_VALGRIND
    int stack_id;
#endif
  } _;
  dfk_t* dfk;
  void* userdata;
} dfk_coro_t;


/**
 * Initialize dfk context with default settings.
 */
int dfk_init(dfk_t* dfk);


/**
 * Cleanup resources allocated for dfk context
 *
 * @pre dfk != NULL
 */
int dfk_free(dfk_t* dfk);


/**
 * Start a new coroutine
 *
 * Resources allocated to coroutine will be automatically released upon completion.
 *
 * @pre dfk != NULL
 * @pre ep != NULL
 */
dfk_coro_t* dfk_run(dfk_t* dfk, void (*ep)(dfk_coro_t*, void*), void* arg);


/**
 * Set name of the coroutine.
 *
 * if DFK_NAMED_COROUTINES is disabled, function returns dfk_err_ok and has no effect
 */
int dfk_coro_name(dfk_coro_t* coro, const char* fmt, ...);


/**
 * Switch execution context to another coroutine
 */
int dfk_yield(dfk_coro_t* from, dfk_coro_t* to);


/**
 * Start dfk working cycle.
 *
 * @pre dfk != NULL
 */
int dfk_work(dfk_t* dfk);


/**
 * Returns string representation of the error code @p err.
 *
 * If system error has occured (err = dfk_err_sys), strerror(dfk_t.sys_errno) is returned.
 */
const char* dfk_strerr(dfk_t* dfk, int err);

/** @} */

