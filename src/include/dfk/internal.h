/**
 * @file dfk/internal.h
 * Miscellaneous functions and macros for internal use only.
 *
 * @copyright
 * Copyright (c) 2015-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dfk/config.h>
#include <dfk/context.h>
#include <dfk/log.h>
#include <dfk/scheduler.h>
#include <dfk/eventloop.h>

#define DFK_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DFK_UNUSED(x) (void) (x)

#define DFK_MAX(x, y) ((x) > (y) ? (x) : (y))
#define DFK_MIN(x, y) ((x) < (y) ? (x) : (y))
#define DFK_SWAP(type, x, y) \
{ \
  type tmp = (x); \
  (x) = (y); \
  (y) = tmp; \
}

/**
 * Returns a pointer to struct from a pointer to data member
 *
 * Inspired by `container_of' from Linux kernel
 * (https://www.fsl.cs.sunysb.edu/kernel-api/re85.html)
 */
#define DFK_CONTAINER_OF(ptr, type, member) \
  ((type*)((char*) (ptr) - offsetof(type, member)))

#if DFK_LOGGING
/**
 * Formats and logs a message
 *
 * @note
 * A cheat to suppress -Waddress warning:
 * instead of straightforward "if ((dfk)..." we write
 * "if (((void*) (dfk) != NULL)..."
 * Suggested by http://stackoverflow.com/a/27048575
 */
#define DFK_LOG(dfk, channel, ...) \
do { \
  if (((void*) (dfk) != NULL) && (dfk)->log) {\
    char msg[512] = {0};\
    int printed;\
    printed = snprintf(msg, sizeof(msg), "%s (%s:%d) ", __func__, DFK_FILENAME, __LINE__);\
    snprintf(msg + printed , sizeof(msg) - printed, __VA_ARGS__);\
    (dfk)->log((dfk), channel, msg);\
  } \
} while (0)
#else
#define DFK_LOG(dfk, channel, ...) \
{ \
  DFK_UNUSED(dfk); \
  DFK_UNUSED(channel); \
}
#endif

#define DFK_ERROR(dfk, ...) DFK_LOG((dfk), dfk_log_error, __VA_ARGS__)
#define DFK_WARNING(dfk, ...) DFK_LOG((dfk), dfk_log_warning, __VA_ARGS__)
#define DFK_INFO(dfk, ...) DFK_LOG((dfk), dfk_log_info, __VA_ARGS__)

#if DFK_DEBUG
#define DFK_DBG(dfk, ...) DFK_LOG((dfk), dfk_log_debug, __VA_ARGS__)
#else
#define DFK_DBG(dfk, ...) DFK_UNUSED((dfk))
#endif

/**
 * Log failed system call and set dfk.sys_errno respectively
 *
 * @note dfk expression is evaluated more than once, so it should not have side
 * effects.
 */
#define DFK_ERROR_SYSCALL(dfk, syscall) \
{ \
  DFK_ERROR((dfk), "{%p} " syscall " failed, errno=%d %s", \
      (void*) (dfk), errno, strerror(errno)); \
  (dfk)->sys_errno = errno; \
}

#define DFK_STRINGIFY(D) DFK_STR__(D)
#define DFK_STR__(D) #D

/**
 * Returns number of the elements in a fixed-size array
 */
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

/* Same as DFK_SYSCALL, but jump to label instead of return */
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

/**
 * @see dfk__this_fiber
 */
#define DFK_THIS_FIBER(dfk) \
  dfk__this_fiber((dfk)->_scheduler)

/**
 * @see dfk__suspend
 */
#define DFK_SUSPEND(dfk) \
  dfk__suspend((dfk)->_scheduler);

/**
 * @see dfk__postpone
 */
#define DFK_POSTPONE(dfk) \
  dfk__postpone((dfk)->_scheduler);

/**
 * @see dfk__io
 */
#define DFK_IO(dfk, socket, flags) \
  dfk__io((dfk)->_eventloop, (socket), (flags));

/**
 * @see dfk__iosuspend
 */
#define DFK_IOSUSPEND(dfk) \
  dfk__iosuspend((dfk)->_scheduler)

/**
 * @see dfk__ioresume
 */
#define DFK_IORESUME(fiber) \
  dfk__ioresume((fiber)->dfk->_scheduler, (fiber))

/**
 * @see dfk__resume
 */
#define DFK_RESUME(fiber) \
  dfk__resume((fiber)->dfk->_scheduler, (fiber))

#if DFK_THREAD_SANITIZER
#define DFK_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define DFK_NO_SANITIZE_THREAD
#endif

/**
 * Magic value to fill pointer which value is considered unspecified
 */
#define DFK_PDEADBEEF ((void*) 0xDEADBEEF)

/**
 * Magic value to fill integers which value is considered unspecified
 */
#define DFK_DEADBEEF ((uint32_t) 0xDEADBEEF)

#if DFK_DEBUG
/**
 * Evaluates to the argument if DFK_DEBUG is enabled
 */
#define DFK_IF_DEBUG(expr) expr
#else
#define DFK_IF_DEBUG(...)
#endif

