#include <assert.h>
#include <dfk/core.h>
#include <dfk/coro.h>
#include "common.h"


static dfk_coro_t root;

static void coro_main(void* arg)
{
  dfk_coro_t* coro = (dfk_coro_t*) arg;
  coro->_.func(coro->_.arg);
  coro->_.terminated |= 1;
  DFK_DEBUG(coro->_.context,
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
  assert(coro);
  assert(context);

  coro->_.context = context;
  if (!stack_size) {
    stack_size = context->default_coro_stack_size;
  }
  coro->_.stack = DFK_MALLOC(context, stack_size);
  if (coro->_.stack == NULL) {
    DFK_ERROR(context, "unable to allocate stack of size %lu for a new coro",
        stack_size);
    return dfk_err_out_of_memory;
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
      (void*) coro, stack_size);
  coro_create(&coro->_.ctx, coro_main, coro, coro->_.stack, coro->_.stack_size);
  context->_.current_coro = coro;
  coro_transfer(&coro->_.parent->_.ctx, &coro->_.ctx);
  return dfk_err_ok;
}

int dfk_coro_yield(dfk_coro_t* from, dfk_coro_t* to)
{
  assert(to);
  if (from->_.context != to->_.context && from != &root && to != &root) {
    DFK_ERROR(from->_.context,
        "unable to switch from %p to %p, "
        "corotines belong to different contexts",
        (void*) from, (void*) to);
    return dfk_err_diff_context;
  }
  from->_.context->_.current_coro = to;
  DFK_DEBUG(from->_.context, "context switch %p -> %p",
      (void*) from, (void*) to);
  coro_transfer(&from->_.ctx, &to->_.ctx);
  return dfk_err_ok;
}

int dfk_coro_yield_parent(dfk_coro_t* coro)
{
  return dfk_coro_yield(coro, coro->_.parent);
}

int dfk_coro_yield_to(dfk_context_t* ctx, dfk_coro_t* to)
{
  assert(ctx);
  return dfk_coro_yield(ctx->_.current_coro, to);
}

int dfk_coro_join(dfk_coro_t* coro)
{
  int err;
  dfk_context_t* context;

  assert(coro);

  context = coro->_.context;
  DFK_DEBUG(context, "join coroutine %p", (void*) coro);

  if (coro == coro->_.context->_.current_coro) {
    DFK_ERROR(context, "unable to self-join %p", (void*) coro);
    return dfk_err_badarg;
  }

  while (!coro->_.terminated) {
    dfk_coro_t* current = coro->_.context->_.current_coro;

    if ((err = dfk_coro_yield(current, coro)) != dfk_err_ok) {
      return err;
    }
  }

  DFK_DEBUG(context,
      "coroutine %p terminated, cleanup resources",
      (void*) coro);

  (void) coro_destroy(&coro->_.ctx);
  DFK_FREE(coro->_.context, coro->_.stack);

  return dfk_err_ok;
}
