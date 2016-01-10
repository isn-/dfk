#pragma once
#include <libcoro/coro.h>
#include <dfk/config.h>
#include <dfk/context.h>
#include <dfk/buf.h>

struct dfk_coro_t;

typedef struct dfk_coro_t {
  struct {
    dfk_context_t* context;
    struct coro_context ctx;
    struct dfk_coro_t* parent;
    void (*func)(void*);
    void* arg;
    char* stack;
    size_t stack_size;
    int terminated : 1;
  } _;
} dfk_coro_t;


int dfk_coro_run(
    dfk_coro_t* coro,
    dfk_context_t* context,
    void (*func)(void*),
    void* arg,
    size_t stack_size);

int dfk_coro_yield(dfk_coro_t* from, dfk_coro_t* to);
int dfk_coro_yield_to(dfk_context_t* ctx, dfk_coro_t* to);
int dfk_coro_yield_parent(dfk_coro_t* from);
int dfk_coro_join(dfk_coro_t* coro);

