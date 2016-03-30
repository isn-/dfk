/**
 * @file dfk/core.h
 * Basic definitions
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
#include <dfk/context.h>


/**
 * Error codes returned by dfk functions
 */
typedef enum {
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
   * An operation on objects that belong to different contexts is requested
   */
  dfk_err_context,

  /**
   * System error, see dfk_context_t.sys_errno
   */
  dfk_err_sys,

  /**
   * The operation is already in progress
   */
  dfk_err_inprog,

  /**
   * Function can not be called outside/inside of a coroutine.
   *
   * See function documentation for usage restrictions.
   */
  dfk_err_coro,

  /**
   * Unexpected behaviour, e.g. unreachable code executed
   *
   * Please submit a bug report if API returns this error code.
   * https://github.com/isn-/dfk/issues/new
   */
  dfk_err_panic
} dfk_error_e;

int dfk_strerr(dfk_context_t* ctx, int err, char** msg);

/**
 * Logging stream
 */
typedef enum {
  /**
   * Error message
   */
  dfk_log_error = 0,
  /**
   * Informational message
   */
  dfk_log_info = 1,
  /**
   * Debug
   *
   * A very verbose logging level. Disabled by default, can be enabled
   * by setting DFK_ENABLE_DEBUG compile-time flag.
   */
  dfk_log_debug = 2
} dfk_log_e;


typedef struct {
  char* data;
  size_t size;
} dfk_iovec_t;

