/**
 * @file dfk/internal.h
 * Miscellaneous functions and macros for internal use only.
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
#pragma GCC diagnostic push

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <dfk/config.h>

#define DFK_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DFK_UNUSED(x) (void) (x)

#define DFK_MALLOC(dfk, nbytes) (dfk)->malloc((dfk), nbytes)
#define DFK_FREE(dfk, p) (dfk)->free((dfk), p)
#define DFK_REALLOC(dfk, p, nbytes) (dfk)->realloc((dfk), p, nbytes)

#define DFK_MAX(x, y) ((x) > (y) ? (x) : (y))
#define DFK_MIN(x, y) ((x) < (y) ? (x) : (y))
#define DFK_SWAP(type, x, y) \
{ \
  type tmp = (x); \
  (x) = (y); \
  (y) = tmp; \
}

/*
 * A cheat to suppress -Waddress warning:
 * instead of straightforward "if ((dfk)..." we write
 * "if (((void*) (dfk) != NULL)..."
 * Suggested by http://stackoverflow.com/a/27048575
 */
#define DFK_LOG(dfk, channel, ...) \
if (((void*) (dfk) != NULL) && (dfk)->log) {\
  char msg[512] = {0};\
  int printed;\
  printed = snprintf(msg, sizeof(msg), "%s (%s:%d) ", __func__, DFK_FILENAME, __LINE__);\
  snprintf(msg + printed , sizeof(msg) - printed, __VA_ARGS__);\
  (dfk)->log((dfk), channel, msg);\
}

#define DFK_ERROR(dfk, ...) DFK_LOG((dfk), dfk_log_error, __VA_ARGS__)
#define DFK_WARNING(dfk, ...) DFK_LOG((dfk), dfk_log_warning, __VA_ARGS__)
#define DFK_INFO(dfk, ...) DFK_LOG((dfk), dfk_log_info, __VA_ARGS__)

#if DFK_DEBUG
#define DFK_DBG(dfk, ...) DFK_LOG((dfk), dfk_log_debug, __VA_ARGS__)
#else
#define DFK_DBG(dfk, ...) DFK_UNUSED((dfk))
#endif

#define DFK_STRINGIFY(D) DFK_STR__(D)
#define DFK_STR__(D) #D

#define DFK_SIZE(c) (sizeof((c)) / sizeof((c)[0]))

#define DFK_CALL(dfk, c) \
{ \
  int err; \
  if ((err = (c)) != dfk_err_ok) { \
    DFK_ERROR((dfk), "call \"" DFK_STRINGIFY(c) "\" " \
        "failed with code %s", dfk_strerr((dfk), err)); \
    return err; \
  } \
}

/* Same as DFK_CALL, but does not return error code */
#define DFK_CALL_RVOID(dfk, c) \
{ \
  int err; \
  if ((err = (c)) != dfk_err_ok) { \
    DFK_ERROR((dfk), "call \"" DFK_STRINGIFY(c) "\" " \
        "failed with code %s", dfk_strerr((dfk), err)); \
    return; \
  } \
}

/* Same as DFK_CALL, but jump to label instead of return */
#define DFK_CALL_GOTO(dfk, c, label) \
{ \
  int err; \
  if ((err = (c)) != dfk_err_ok) { \
    DFK_ERROR((dfk), "call \"" DFK_STRINGIFY(c) "\" " \
        "failed with code %s", dfk_strerr((dfk), err)); \
    goto label; \
  } \
}

#define DFK_SYSCALL(dfk, c) \
{ \
  int err; \
  if ((err = (c)) != 0) {\
    DFK_ERROR((dfk), "syscall \"" DFK_STRINGIFY(c) "\" " \
        "failed with code %s", dfk_strerr((dfk), err)); \
    (dfk)->sys_errno = err; \
    return dfk_err_sys; \
  } \
}

/* Same as DFK_SYSCALL, but does not return error code */
#define DFK_SYSCALL_RVOID(dfk, c) \
{ \
  int err; \
  if ((err = (c)) != 0) {\
    DFK_ERROR((dfk), "syscall \"" DFK_STRINGIFY(c) "\" " \
        "failed with code %s", dfk_strerr((dfk), err)); \
    (dfk)->sys_errno = err; \
    return; \
  } \
}

/* Same as DFK_CALL, but jump to label instead of return */
#define DFK_SYSCALL_GOTO(dfk, c, label) \
{ \
  int err; \
  if ((err = (c)) != 0) {\
    DFK_ERROR((dfk), "syscall \"" DFK_STRINGIFY(c) "\" " \
        "failed with code %s", dfk_strerr((dfk), err)); \
    (dfk)->sys_errno = err; \
    goto label; \
  } \
}

#define DFK_THIS_CORO(dfk) (dfk)->_current

#define DFK_YIELD(dfk) \
{ \
  DFK_CALL((dfk), dfk_yield(DFK_THIS_CORO((dfk)), (dfk)->_scheduler)); \
}

#define DFK_YIELD_RVOID(dfk) \
{ \
  DFK_CALL_RVOID((dfk), dfk_yield(DFK_THIS_CORO((dfk)), (dfk)->_scheduler)); \
}

#define DFK_POSTPONE(dfk) \
{ \
  dfk_list_append(&(dfk)->_pending_coros, (dfk_list_hook_t*) DFK_THIS_CORO((dfk))); \
  DFK_YIELD((dfk)); \
}

#define DFK_POSTPONE_RVOID(dfk) \
{ \
  dfk_list_append(&(dfk)->_pending_coros, (dfk_list_hook_t*) DFK_THIS_CORO((dfk))); \
  DFK_YIELD_RVOID((dfk)); \
}

#define DFK_IO(dfk) \
{ \
  dfk_coro_t* self = DFK_THIS_CORO((dfk)); \
  dfk_list_append(&(dfk)->_iowait_coros, (dfk_list_hook_t*) self); \
  DFK_YIELD((dfk)); \
}

#define DFK_RESUME(dfk, coro) \
{ \
  dfk_list_append(&(dfk)->_pending_coros, (dfk_list_hook_t*) (coro)); \
}

#define DFK_IO_RESUME(dfk, yieldback) \
{ \
  dfk_list_erase(&(dfk)->_iowait_coros, (dfk_list_hook_t*) (yieldback)); \
  DFK_RESUME((dfk), yieldback); \
}

#if DFK_THREAD_SANITIZER
#define DFK_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define DFK_NO_SANITIZE_THREAD
#endif

#pragma GCC diagnostic pop

