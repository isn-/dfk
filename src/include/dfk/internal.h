/**
 * @file dfk/internal.h
 * Miscellaneous functions and macros for internal use only.
 *
 * @copyright
 * Copyright (c) 2015-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#pragma GCC diagnostic push

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <dfk/context.h>
#include <dfk/config.h>
#include <dfk/log.h>

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

#if DFK_LOGGING
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

/*
 * A macro to use in scheduler implementation
 */
#define DFK_SCHED(dfk, from, to) \
{ \
  (dfk)->_current = (to); \
  dfk_yield((from), (to)); \
}

#if DFK_THREAD_SANITIZER
#define DFK_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define DFK_NO_SANITIZE_THREAD
#endif

typedef struct dfk_epoll_arg_t {
  void (*callback)(uint32_t events, void* arg0, void* arg1, void* arg2, void* arg3);
  void* arg0;
  void* arg1;
  void* arg2;
  void* arg3;
} dfk_epoll_arg_t;


#define DFK_PDEADBEEF ((void*) 0xDEADBEEF)
#define DFK_DEADBEEF ((uint32_t) 0xDEADBEEF)

#if DFK_DEBUG
#define DFK_IF_DEBUG(expr) expr
#else
#define DFK_IF_DEBUG(...)
#endif

#ifdef __cplusplus
}
#endif

#pragma GCC diagnostic pop

