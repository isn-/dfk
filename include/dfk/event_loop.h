#pragma once
#include <uv.h>

typedef struct {
  struct {
    uv_loop_t loop;
  } _;
} dfk_event_loop_t;

int dfk_event_loop_init(dfk_event_loop_t* loop);
int dfk_event_loop_free(dfk_event_loop_t* loop);

