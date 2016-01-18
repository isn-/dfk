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
  DFK_DEBUG(CTX(loop), "(%p)", (void*) loop);
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
  err = dfk_coro_run(
      &loop->_.coro,
      loop->_.ctx,
      dfk_event_loop_main,
      loop, 0);
  DFK_DEBUG(CTX(loop), "(%p) dfk_coro_run returned %d", (void*) loop, err);
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

