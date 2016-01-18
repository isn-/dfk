#pragma once
#include <uv.h>
#include <dfk/context.h>
#include <dfk/coro.h>

typedef struct {
  struct {
    uv_loop_t loop;
    dfk_context_t* ctx;
    dfk_coro_t coro;
  } _;
} dfk_event_loop_t;

int dfk_event_loop_init(dfk_event_loop_t* loop, dfk_context_t* ctx);
int dfk_event_loop_free(dfk_event_loop_t* loop);
int dfk_event_loop_run(dfk_event_loop_t* loop);
int dfk_event_loop_join(dfk_event_loop_t* loop);

