/**
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

typedef struct {
  char* data;
  size_t size;
} buf_t;

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DFK_MALLOC(ctx, nbytes) (ctx)->malloc((ctx)->userdata, nbytes)
#define DFK_FREE(ctx, p) (ctx)->free((ctx)->userdata, p)
#define DFK_REALLOC(ctx, p, nbytes) (ctx)->realloc((ctx)->userdata, p, nbytes)

#define DFK_LOG(ctx, channel, ...) \
if ((ctx)->log) {\
  char msg[512] = {0};\
  int printed;\
  printed = snprintf(msg, sizeof(msg), "%s (%s:%d) ", __FUNCTION__, __FILENAME__, __LINE__);\
  snprintf(msg + printed , sizeof(msg) - printed, __VA_ARGS__);\
  (ctx)->log((ctx)->userdata, channel, msg);\
}

#define DFK_ERROR(ctx, ...) DFK_LOG(ctx, 0, __VA_ARGS__)
#define DFK_INFO(ctx, ...) DFK_LOG(ctx, 1, __VA_ARGS__)

#ifdef DFK_ENABLE_DEBUG
#define DFK_DEBUG(ctx, ...) DFK_LOG(ctx, 2, __VA_ARGS__)
#else
#define DFK_DEBUG(...)
#endif

#pragma GCC diagnostic pop

