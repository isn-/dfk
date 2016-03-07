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

#include <assert.h>
#include <dfk/core.h>
#include <dfk/event_loop.h>
#include "common.h"

#define CTX(loop) ((loop)->_.ctx)

int dfk_event_loop_init(dfk_event_loop_t* loop, dfk_context_t* ctx)
{
  if (loop == NULL || ctx == NULL) {
    return dfk_err_badarg;
  }
  DFK_DEBUG(ctx, "(%p)", (void*) loop);
  uv_loop_init(&loop->_.loop);
  loop->_.ctx = ctx;
  loop->_.loop.data = loop;
  return dfk_err_ok;
}

int dfk_event_loop_free(dfk_event_loop_t* loop)
{
  if (loop == NULL) {
    return dfk_err_badarg;
  }
  DFK_DEBUG(CTX(loop), "(%p)", (void*) loop);
  uv_loop_close(&loop->_.loop);
  return dfk_err_ok;
}

static void dfk_event_loop_main(void* arg)
{
  int err;
  dfk_event_loop_t* loop = (dfk_event_loop_t*) arg;
  assert(loop);
  DFK_DEBUG(CTX(loop), "(%p) main routine started", (void*) loop);
  while (uv_loop_alive(&loop->_.loop)) {
    DFK_DEBUG(CTX(loop), "(%p) poll", (void*) loop);
    err = uv_run(&loop->_.loop, UV_RUN_ONCE);
    DFK_DEBUG(CTX(loop), "(%p) uv_run returned %d", (void*) loop, err);
    if (err == 0) {
      /* From libuv documentation:
       * UV_RUN_ONCE: Poll for i/o once. <...> Returns zero when done
       * (no active handles or requests left), or non-zero if more callbacks
       * are expected (meaning you should run the event loop again sometime
       * in the future).
       */
      break;
    }
  }
  DFK_DEBUG(CTX(loop), "(%p) loop is dead", (void*) loop);
}

int dfk_event_loop_run(dfk_event_loop_t* loop)
{
  int err;
  if (loop == NULL) {
    return dfk_err_badarg;
  }
  DFK_INFO(CTX(loop), "(%p) spawning main routine", (void*) loop);
  err = dfk_coro_init(
      &loop->_.coro,
      loop->_.ctx,
      0);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(loop), "(%p) dfk_coro_init returned %d", (void*) loop, err);
    return err;
  }
  err = dfk_coro_run(&loop->_.coro, dfk_event_loop_main, loop);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(loop), "(%p) dfk_coro_run returned %d", (void*) loop, err);
  }
  return err;
}

int dfk_event_loop_join(dfk_event_loop_t* loop)
{
  if (loop == NULL) {
    return dfk_err_badarg;
  }
  DFK_DEBUG(CTX(loop), "(%p)", (void*) loop);
  return dfk_coro_join(&loop->_.coro);
}

