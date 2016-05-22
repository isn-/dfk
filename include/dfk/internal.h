/**
 * @file dfk/internal.h
 * Miscellaneous functions and macros for internal use only.
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <dfk/config.h>

#define DFK_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DFK_UNUSED(x) (void) (x)

#define DFK_MALLOC(dfk, nbytes) (dfk)->malloc((dfk), nbytes)
#define DFK_FREE(dfk, p) (dfk)->free((dfk), p)
#define DFK_REALLOC(dfk, p, nbytes) (dfk)->realloc((dfk), p, nbytes)

#define DFK_LOG(dfk, channel, ...) \
if ((dfk) && (dfk)->log) {\
  char msg[512] = {0};\
  int printed;\
  printed = snprintf(msg, sizeof(msg), "%s (%s:%d) ", __func__, DFK_FILENAME, __LINE__);\
  snprintf(msg + printed , sizeof(msg) - printed, __VA_ARGS__);\
  (dfk)->log((dfk)->userdata, channel, msg);\
}

#define DFK_ERROR(dfk, ...) DFK_LOG(dfk, dfk_log_error, __VA_ARGS__)
#define DFK_WARNING(dfk, ...) DFK_LOG(dfk, dfk_log_warning, __VA_ARGS__)
#define DFK_INFO(dfk, ...) DFK_LOG(dfk, dfk_log_info, __VA_ARGS__)

#ifdef DFK_DEBUG
#undef DFK_DEBUG
#define DFK_DEBUG_ENABLED
#define DFK_DEBUG(dfk, ...) DFK_LOG(dfk, dfk_log_debug, __VA_ARGS__)
#else
#define DFK_DEBUG(dfk, ...) DFK_UNUSED(dfk)
#endif

#define DFK_STRINGIFY(D) DFK_STR__(D)
#define DFK_STR__(D) #D

#define DFK_SIZE(c) (sizeof((c)) / sizeof((c)[0]))

#define DFK_CALL(c) \
{ \
  int err; \
  if ((err = (c)) != dfk_err_ok) { \
    return err; \
  } \
}

#define DFK_SYSCALL(dfk, c) \
{ \
  int err; \
  if ((err = (c)) != 0) {\
    (dfk)->sys_errno = err; \
    return dfk_err_sys; \
  } \
}

#define DFK_THIS_CORO(dfk) (dfk)->_.current

#define DFK_YIELD_EVENTLOOP(dfk) \
  DFK_CALL(dfk_yield(DFK_THIS_CORO((dfk)), (dfk)->_.eventloop));

#pragma GCC diagnostic pop

