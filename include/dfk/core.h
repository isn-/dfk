/**
 * @file dfk/core.h
 * Core definitions
 *
 * @copyright
 * Copyright (c) 2015-2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#include <stddef.h>
#include <pthread.h>
#include <uv.h>
#include <libcoro/coro.h>
#include <dfk/config.h>
#include <dfk/internal/list.h>

/**
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
   * End of file (stream, iterator, etc)
   */
  dfk_err_eof,

  /**
   * Resource is already acquired.
   */
  dfk_err_busy,

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
   * https://github.com/ivochkin/dfk/issues/new
   */
  dfk_err_panic,

  /**
   * Functionality is not implemented yet
   */
  dfk_err_not_implemented,

  /**
   * Floating point, or integer overflow error.
   */
  dfk_err_overflow,

  /**
   * Protocol violation
   *
   * For instance, invalid HTTP request method.
   */
  dfk_err_protocol,

  _dfk_err_total
} dfk_error_e;


typedef struct dfk_iovec_t {
  char* data;
  size_t size;
} dfk_iovec_t;


typedef dfk_iovec_t dfk_buf_t;


/**
 * A struct to associate client data with library object.
 *
 * Compilers complain on casting function pointer to void*, so we
 * use a separate union member when function pointer is required.
 */
typedef union dfk_userdata_t {
  void* data;
  void (*func)(void);
} dfk_userdata_t;


/**
 * dfk library context
 */
typedef struct dfk_t {
  /**
   * @privatesection
   */

  dfk_list_t _pending_coros;
  dfk_list_t _iowait_coros;
  dfk_list_t _terminated_coros;
  struct dfk_coro_t* _current;

  dfk_list_t _http_servers;

  struct dfk_coro_t* _scheduler;
  struct dfk_coro_t* _eventloop;
  uv_loop_t* _uvloop;
  pthread_mutex_t _uvloop_m;
  uv_async_t _stop;
  sig_atomic_t _stopped;

  /**
   * @publicsection
   */

  dfk_userdata_t user;

  /** User-provided memory allocation function */
  void* (*malloc) (struct dfk_t*, size_t);

  /** User-provided memory deallocation function */
  void (*free) (struct dfk_t*, void*);

  /** User-provided memory relocation function */
  void* (*realloc)(struct dfk_t*, void*, size_t);

  /** User-provided logging function */
  void (*log)(struct dfk_t*, int, const char*);

  /**
   * Initial stack size for new coroutines
   *
   * @note default: #DFK_STACK_SIZE
   */
  size_t default_stack_size;

  int sys_errno;
  int dfk_errno;

} dfk_t;


/**
 * Coroutine context
 */
typedef struct dfk_coro_t {
  /**
   * @privatesection
   */

  dfk_list_hook_t _hook;
  struct coro_context _ctx;
  void (*_ep)(struct dfk_coro_t*, void*);
  void* _arg;
#if DFK_NAMED_COROUTINES
  char _name[DFK_COROUTINE_NAME_LENGTH];
#endif
#if DFK_VALGRIND
  int _stack_id;
#endif

  /**
   * @publicsection
   */

  dfk_t* dfk;
  dfk_userdata_t user;
} dfk_coro_t;


/**
 * Initialize dfk context with default settings.
 */
int dfk_init(dfk_t* dfk);


/**
 * Cleanup resources allocated for dfk context
 */
int dfk_free(dfk_t* dfk);


/**
 * Start a new coroutine
 *
 * Resources allocated to coroutine will be automatically released upon completion.
 * @param argsize Size of the arg object. If argsize is set to non-zero value, @p argsize
 * bytes of memory pointed by @p arg will be copied onto the stack of the newly created
 * coroutine and @p ep will be provided with arg copy. Set argsize to zero if arg
 * is guaranteed to be accessible within spawned coroutine lifetime.
 */
dfk_coro_t* dfk_run(dfk_t* dfk, void (*ep)(dfk_coro_t*, void*), void* arg, size_t argsize);


/**
 * Set name of the coroutine.
 *
 * If #DFK_NAMED_COROUTINES is disabled, function returns dfk_err_ok and has no effect
 */
int dfk_coro_name(dfk_coro_t* coro, const char* fmt, ...);


/**
 * Switch execution context to another coroutine
 *
 * @note dfk_yield is a low-level function and is not recommended for everyday usage.
 * Prefer using sync module for higher-level coroutine synchronization techniques.
 */
int dfk_yield(dfk_coro_t* from, dfk_coro_t* to);


/**
 * Start dfk working cycle.
 */
int dfk_work(dfk_t* dfk);


/**
 * Stop dfk working cycle
 */
int dfk_stop(dfk_t* dfk);


/**
 * Returns string representation of the error code @p err.
 *
 * If system error has occured (err == dfk_err_sys), strerror(dfk->sys_errno) is returned.
 */
const char* dfk_strerr(dfk_t* dfk, int err);

/** @} */

