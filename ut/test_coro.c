#include <dfk.h>
#include "ut.h"
#include <stdio.h>

static void inc_int(void* arg)
{
  int* iarg = (int*) arg;
  (*iarg)++;
}


TEST(coro, run_and_join)
{
  int called = 0;
  dfk_context_t* ctx = dfk_default_context();
  dfk_coro_t coro;

  EXPECT(dfk_coro_run(&coro, ctx, inc_int, &called, 0) == dfk_err_ok);
  EXPECT(dfk_coro_join(&coro) == dfk_err_ok);
  EXPECT(called == 1);
}


TEST(coro, multi)
{
  size_t ncalls = 0;
  dfk_context_t* ctx = dfk_default_context();
  dfk_coro_t workers[5];
  size_t ncoros = sizeof(workers) / sizeof(workers[0]);
  size_t i;

  for (i = 0; i < ncoros; ++i) {
    EXPECT(dfk_coro_run(workers +i, ctx, inc_int, &ncalls, 0) == dfk_err_ok);
  }
  for (i = 0; i < ncoros; ++i) {
    EXPECT(dfk_coro_join(workers + i) == dfk_err_ok);
  }
  EXPECT(ncalls == ncoros);
}

typedef struct {
  dfk_coro_t* self;
  dfk_coro_t* other;
  int* flag;
  int state;
} yield_coro_arg;

static void yield_worker(void* parg)
{
  yield_coro_arg* arg = (yield_coro_arg*) parg;
  arg->state = 1;
  dfk_coro_yield_parent(arg->self);
  arg->state = 2;
  if (arg->self == arg->other) {
    dfk_coro_yield_parent(arg->self);
  } else {
    dfk_coro_yield(arg->self, arg->other);
  }
}

static void yield_main(void* parg)
{
  dfk_context_t* ctx = dfk_default_context();
  int* flag = (int*) parg;
  dfk_coro_t workers[3];
  yield_coro_arg args[3];
  size_t i;
  size_t ncoros = sizeof(workers) / sizeof(workers[0]);

  (void) parg;

  for (i = 0; i < ncoros; ++i) {
    args[i].self = workers + i;
    if (i == ncoros - 1) {
      args[i].other = args[i].self;
    } else {
      args[i].other = workers + ((i + 1) % ncoros);
    }
    args[i].flag = flag;
    args[i].state = 0;
  }

  for (i = 0; i < ncoros; ++i) {
    dfk_coro_run(workers + i, ctx, yield_worker, args + i, 0);
  }

  for (i = 0; i < ncoros; ++i) {
    EXPECT(args[i].state == 1);
  }

  dfk_coro_yield_to(ctx, workers);

  for (i = 0; i < ncoros; ++i) {
    EXPECT(args[i].state == 2);
  }

  for (i = 0; i < ncoros; ++i) {
    dfk_coro_join(workers + i);
  }

  *flag = 1;
}

TEST(coro, yield)
{
  int flag = 0;
  dfk_coro_t maincoro;
  dfk_context_t* ctx = dfk_default_context();
  dfk_coro_run(&maincoro, ctx, yield_main, &flag, 0);
  dfk_coro_join(&maincoro);
  EXPECT(flag == 1);
}
