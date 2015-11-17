#include <dfk.h>
#include <assert.h>

#define BUFFER_USAGE_STACK "stack"

int dfk_coro_init(
    dfk_coro_t* coro,
    dfk_bufman_t* bm,
    void (*func)(void*),
    void* arg,
    size_t stack_size)
{
  dfk_buf_t* stack;
  int err;
  dfk_bufman_alloc_request_t req;
  if (!stack_size) {
    stack_size = DFK_DEFAULT_STACK_SIZE;
  }
  req.size = stack_size;
  req.lifetime = 0;
  req.usage = "stack";
  req.usagelen = 5;
  if ((err = dfk_bufman_alloc(bm, &req, &stack)) != dfk_err_ok) {
    return err;
  }
  coro_create(&coro->_.ctx, func, arg, stack->data, stack->size);
  coro->stack = stack;
  return dfk_err_ok;
}

int dfk_coro_free(dfk_coro_t* coro, dfk_bufman_t* bm)
{
  int err;
  assert(coro);
  assert(bm);
  err = dfk_bufman_release(bm, coro->stack);
  /* suppress statement with no effect warning */
  (void) coro_destroy(&coro->_.ctx);
  return err;
}

int dfk_coro_yield(dfk_coro_t* from, dfk_coro_t* to)
{
  assert(from);
  assert(to);
  coro_transfer(&from->_.ctx, &to->_.ctx);
  return dfk_err_ok;
}

static coro_context main_coro;

int dfk_coro_start(dfk_coro_t* coro)
{
  assert(coro);
  coro_transfer(&main_coro, &coro->_.ctx);
  return dfk_err_ok;
}

int dfk_coro_return(dfk_coro_t* coro)
{
  assert(coro);
  coro_transfer(&coro->_.ctx, &main_coro);
  return dfk_err_ok;
}

