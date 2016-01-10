#include <assert.h>
#include <dfk/core.h>
#include <dfk/event_loop.h>

int dfk_event_loop_init(dfk_event_loop_t* loop, dfk_context_t* ctx)
{
  assert(loop);
  (void) ctx;
  uv_loop_init(&loop->_.loop);
  return dfk_err_ok;
}

int dfk_event_loop_free(dfk_event_loop_t* loop)
{
  assert(loop);
  uv_loop_close(&loop->_.loop);
  return dfk_err_ok;
}

int dfk_event_loop_run(dfk_event_loop_t* loop)
{
  assert(loop);
  return dfk_err_ok;
}
