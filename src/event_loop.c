#include <assert.h>
#include <dfk.h>

int dfk_event_loop_init(dfk_event_loop_t* loop)
{
  assert(loop);
  uv_loop_init(&loop->_.loop);
  return dfk_err_ok;
}

int dfk_event_loop_free(dfk_event_loop_t* loop)
{
  assert(loop);
  uv_loop_close(&loop->_.loop);
  return dfk_err_ok;
}

