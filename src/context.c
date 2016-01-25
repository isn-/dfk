/**
 * @author Stanislav Ivochkin
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dfk/core.h>
#include <dfk/config.h>
#include <dfk/context.h>

static void default_log(void* ud, int channel, const char* msg)
{
  char strchannel[5] = {0};
  (void) ud;
  switch(channel) {
    case dfk_log_error: memcpy(strchannel, "error", 5); break;
    case dfk_log_info: memcpy(strchannel, " info", 5); break;
    case dfk_log_debug: memcpy(strchannel, "debug", 5); break;
    default: snprintf(strchannel, sizeof(strchannel), "%5d", channel);
  }
  /* At most 512 bytes will printed at once */
  printf("[%.5s] %.503s\n", strchannel, msg);
}

static void* default_malloc(void* ud, size_t size)
{
  (void) ud;
  return malloc(size);
}

static void default_free(void* ud, void* p)
{
  (void) ud;
  free(p);
}

static void* default_realloc(void* ud, void* p, size_t size)
{
  (void) ud;
  return realloc(p, size);
}

static dfk_context_t default_context = {
  {NULL},
  NULL,
  default_malloc,
  default_free,
  default_realloc,
  default_log,
  DFK_DEFAULT_STACK_SIZE,
  0
};

dfk_context_t* dfk_default_context(void)
{
  return &default_context;
}

int dfk_context_init(dfk_context_t* ctx)
{
  if (ctx == NULL) {
    return dfk_err_badarg;
  }
  ctx->_.current_coro = NULL;
  ctx->userdata = NULL;
  ctx->malloc = default_malloc;
  ctx->free = default_free;
  ctx->realloc = default_realloc;
  ctx->log = default_log;
  ctx->default_coro_stack_size = DFK_DEFAULT_STACK_SIZE;
  ctx->sys_errno = 0;
  return dfk_err_ok;
}

