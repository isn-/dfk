#pragma once
#include <dfk/config.h>
#include <dfk/buf.h>
#include <dfk/bufman.h>
#include <libcoro/coro.h>

typedef struct {
  struct {
    struct coro_context ctx;
  } _;
  dfk_buf_t* stack;
} dfk_coro_t;

int dfk_coro_init(
    dfk_coro_t* coro,
    dfk_bufman_t* bm,
    void (*func)(void*),
    void* arg,
    size_t stack_size);
int dfk_coro_free(dfk_coro_t* coro, dfk_bufman_t* bm);
int dfk_coro_yield(dfk_coro_t* from, dfk_coro_t* to);
int dfk_coro_start(dfk_coro_t* coro);
int dfk_coro_return(dfk_coro_t* coro);

