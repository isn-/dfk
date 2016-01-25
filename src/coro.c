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

#include <dfk/core.h>
#include <dfk/coro.h>
#include "common.h"

#define CTX(coro) ((coro)->_.context)


static dfk_coro_t root;

static void dfk_coro_main(void* arg)
{
  dfk_coro_t* coro = (dfk_coro_t*) arg;
  coro->_.func(coro->_.arg);
  coro->_.terminated |= 1;
  DFK_DEBUG(CTX(coro),
      "coroutine %p func terminated, yield parent",
      (void*) coro);
  dfk_coro_yield_parent(coro);
}

int dfk_coro_run(
    dfk_coro_t* coro,
    dfk_context_t* context,
    void (*func)(void*),
    void* arg,
    size_t stack_size)
{
  if (coro == NULL || context == NULL) {
    return dfk_err_badarg;
  }

  coro->_.context = context;
  if (!stack_size) {
    stack_size = context->default_coro_stack_size;
  }
  coro->_.stack = DFK_MALLOC(context, stack_size);
  if (coro->_.stack == NULL) {
    DFK_ERROR(context, "unable to allocate stack of size %lu for a new coro",
        (unsigned long) stack_size);
    return dfk_err_nomem;
  }
  coro->_.stack_size = stack_size;
  coro->_.func = func;
  coro->_.arg = arg;
  if (context->_.current_coro == NULL) {
    context->_.current_coro = &root;
  }
  coro->_.parent = context->_.current_coro;
  coro->_.terminated = 0;
  DFK_INFO(context, "spawn new coroutine %p, stack size %lu bytes",
      (void*) coro, (unsigned long) stack_size);
  coro_create(&coro->_.ctx, dfk_coro_main, coro, coro->_.stack, coro->_.stack_size);
  return dfk_err_ok;
}

int dfk_coro_yield(dfk_coro_t* from, dfk_coro_t* to)
{
  dfk_context_t* context;
  if (from == NULL || to == NULL) {
    return dfk_err_badarg;
  }
  context = from->_.context ? from->_.context : to->_.context;
  if (from->_.context != to->_.context && from != &root && to != &root) {
    DFK_ERROR(context,
        "unable to switch from %p to %p, "
        "corotines belong to different contexts",
        (void*) from, (void*) to);
    return dfk_err_context;
  }
  DFK_DEBUG(context, "(%p) context switch -> (%p)",
      (void*) from, (void*) to);
  context->_.current_coro = to;
  coro_transfer(&from->_.ctx, &to->_.ctx);
  return dfk_err_ok;
}

int dfk_coro_yield_parent(dfk_coro_t* coro)
{
  return dfk_coro_yield(coro, coro->_.parent);
}

int dfk_coro_yield_to(dfk_context_t* ctx, dfk_coro_t* to)
{
  if (ctx == NULL) {
    return dfk_err_badarg;
  }
  return dfk_coro_yield(ctx->_.current_coro, to);
}

int dfk_coro_join(dfk_coro_t* coro)
{
  int err;
  dfk_context_t* context;

  if (coro == NULL) {
    return dfk_err_badarg;
  }

  context = coro->_.context;
  DFK_DEBUG(context, "(%p) join coroutine", (void*) coro);

  if (coro == coro->_.context->_.current_coro) {
    DFK_ERROR(context, "(%p) unable to self-join", (void*) coro);
    return dfk_err_badarg;
  }

  while (!coro->_.terminated) {
    dfk_coro_t* current = coro->_.context->_.current_coro;

    if ((err = dfk_coro_yield(current, coro)) != dfk_err_ok) {
      return err;
    }
  }

  DFK_DEBUG(context,
      "(%p) terminated, cleanup resources",
      (void*) coro);
  (void) coro_destroy(&coro->_.ctx);
  DFK_DEBUG(context, "(%p) free memory allocated for stack", (void*) coro)
  DFK_FREE(coro->_.context, coro->_.stack);

  return dfk_err_ok;
}

