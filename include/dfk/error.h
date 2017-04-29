/**
 * @file dfk/error.h
 * Defines error codes and dfk_strerror function.
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/context.h>

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

  /**
   * Timeout has expired
   */
  dfk_err_timeout,

  _dfk_err_total
} dfk_error_e;

/**
 * Returns string representation of the error code @p err.
 *
 * If system error has occured (err == dfk_err_sys), strerror(dfk->sys_errno)
 * is returned.
 */
const char* dfk_strerr(dfk_t* dfk, int err);

