#include <dfk.h>
#include "ut.h"
#include <stdio.h>

typedef struct {
  dfk_coro_t* coro;
  int* arg;
} corofunc_arg_t;

static void corofunc(void* arg)
{
  corofunc_arg_t* coroarg = (corofunc_arg_t*) arg;
  *coroarg->arg = 1;
  dfk_coro_return(coroarg->coro);
}


TEST(coroutine, create)
{
  int called = 0;
  dfk_coro_t coro;
  corofunc_arg_t arg;
  dfk_bufman_t bm;
  EXPECT(dfk_bufman_init(&bm) == dfk_err_ok);
  arg.arg = &called;
  arg.coro = &coro;
  EXPECT(dfk_coro_init(&coro, &bm, corofunc, &arg, 0) == dfk_err_ok);
  EXPECT(dfk_coro_start(&coro) == dfk_err_ok);
  EXPECT(called == 1);
  EXPECT(dfk_coro_free(&coro, &bm) == dfk_err_ok);
  EXPECT(dfk_bufman_free(&bm) == dfk_err_ok);
}

