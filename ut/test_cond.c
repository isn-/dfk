/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/cond.h>
#include <dfk/internal.h>
#include <dfk/scheduler.h>
#include <ut.h>

typedef struct {
  dfk_t dfk;
  dfk_mutex_t mutex;
  dfk_cond_t cv;
  int state;
} cond_fixture_t;

static void cond_fixture_setup(cond_fixture_t* f)
{
  f->state = 0;
  dfk_init(&f->dfk);
}

static void cond_fixture_teardown(cond_fixture_t* f)
{
  dfk_free(&f->dfk);
}

static void init_free_main(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(arg);
  dfk_cond_t cv;
  dfk_cond_init(&cv, fiber->dfk);
  dfk_cond_free(&cv);
}

TEST_F(cond_fixture, cond, init_free)
{
  dfk_work(&fixture->dfk, init_free_main, NULL, 0);
}

static void signal_empty_waitqueue_main(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(arg);
  dfk_cond_t cv;
  dfk_cond_init(&cv, fiber->dfk);
  dfk_cond_signal(&cv);
  dfk_cond_free(&cv);
}

TEST_F(cond_fixture, cond, signal_empty_waitqueue)
{
  dfk_work(&fixture->dfk, signal_empty_waitqueue_main, NULL, 0);
}

static void broadcast_empty_waitqueue_main(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(arg);
  dfk_cond_t cv;
  dfk_cond_init(&cv, fiber->dfk);
  dfk_cond_broadcast(&cv);
  dfk_cond_free(&cv);
}

TEST_F(cond_fixture, cond, broadcast_empty_waitqueue)
{
  dfk_work(&fixture->dfk, broadcast_empty_waitqueue_main, NULL, 0);
}

static void signal_single_wait_fiber_0(dfk_fiber_t* fiber, void* arg)
{
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_lock(&f->mutex);
  f->state = 1;
  dfk_cond_wait(&f->cv, &f->mutex);
  f->state = 4;
  EXPECT(f->mutex._owner == fiber);
  dfk_mutex_unlock(&f->mutex);
  dfk_cond_free(&f->cv);
  dfk_mutex_free(&f->mutex);
}


static void signal_single_wait_fiber_1(dfk_fiber_t* fiber, void* arg)
{
  dfk_t* dfk = fiber->dfk;
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_lock(&f->mutex);
  f->state = 2;
  dfk__postpone(dfk->_scheduler);
  dfk_cond_signal(&f->cv);
  f->state = 3;
  /* fiber 0 should not run because fiber 1 still owns the mutex */
  dfk__postpone(dfk->_scheduler);
  EXPECT(f->state == 3);
  dfk_mutex_unlock(&f->mutex);
}

static void signal_single_wait_main(dfk_fiber_t* fiber, void* arg)
{
  dfk_t* dfk = fiber->dfk;
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_init(&f->mutex, dfk);
  dfk_cond_init(&f->cv, dfk);
  EXPECT(dfk_run(dfk, signal_single_wait_fiber_0, arg, 0));
  while (f->state != 1) {
    dfk__postpone(dfk->_scheduler);
  }
  EXPECT(dfk_run(dfk, signal_single_wait_fiber_1, arg, 0));
}

/**
 * States:
 * 1 - fiber_0 obtained a lock on the mutex and started waiting on cv
 * 2 - fiber_1 obtained a lock on the mutex
 * 3 - fiber_1 signalled, but haven't unlocked the mutex
 * 4 - fiber_0 returned from dfk_cond_wait
 */
TEST_F(cond_fixture, cond, signal_single_wait)
{
  dfk_work(&fixture->dfk, signal_single_wait_main, fixture, 0);
}


/*
static void ut_cond_wait_unlock_mutex_coro0(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  EXPECT_OK(dfk_mutex_init(d->mutex, fiber->dfk));
  EXPECT_OK(dfk_cond_init(d->cv, fiber->dfk));
  EXPECT_OK(dfk_mutex_lock(d->mutex));
  DFK_POSTPONE_RVOID(fiber->dfk);
  EXPECT_OK(dfk_cond_wait(d->cv, d->mutex));
  EXPECT_OK(dfk_mutex_unlock(d->mutex));
  EXPECT_OK(dfk_cond_free(d->cv));
  EXPECT_OK(dfk_mutex_free(d->mutex));
}


static void ut_cond_wait_unlock_mutex_coro1(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  DFK_UNUSED(fiber);
  EXPECT_OK(dfk_mutex_lock(d->mutex));
  EXPECT_OK(dfk_mutex_unlock(d->mutex));
}


static void ut_cond_wait_unlock_mutex_coro2(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  DFK_POSTPONE_RVOID(fiber->dfk);
  DFK_POSTPONE_RVOID(fiber->dfk);
  EXPECT_OK(dfk_cond_signal(d->cv));
}


TOST_F(cond_fixture, cond, wait_unlock_mutex)
{
  dfk_mutex_t mutex;
  dfk_cond_t cv;
  ut_cond_ssw arg = {&mutex, &cv};
  EXPECT(dfk_run(&fixture->dfk, ut_cond_wait_unlock_mutex_coro0, &arg, 0));
  EXPECT(dfk_run(&fixture->dfk, ut_cond_wait_unlock_mutex_coro1, &arg, 0));
  EXPECT(dfk_run(&fixture->dfk, ut_cond_wait_unlock_mutex_coro2, &arg, 0));
  EXPECT_OK(dfk_work(&fixture->dfk));
}


typedef struct ut_cond_smw {
  dfk_mutex_t* mutex;
  dfk_cond_t* cv;
  int* state;
} ut_cond_smw;


#define CHANGE_STATE(pstate, from, to) \
{ \
  EXPECT(*(pstate) == (from)); \
  *(pstate) = (to); \
}


static void ut_cond_signal_multi_wait_coro0(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  EXPECT_OK(dfk_mutex_init(d->mutex, fiber->dfk))
  EXPECT_OK(dfk_cond_init(d->cv, fiber->dfk));
  EXPECT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 0, 1);
  EXPECT_OK(dfk_cond_wait(d->cv, d->mutex));
  CHANGE_STATE(d->state, 3, 4);
  EXPECT(d->mutex->_owner == DFK_THIS_CORO(fiber->dfk));
  EXPECT_OK(dfk_mutex_unlock(d->mutex))
}


static void ut_cond_signal_multi_wait_coro1(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  EXPECT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 1, 2);
  EXPECT_OK(dfk_cond_wait(d->cv, d->mutex));
  EXPECT(d->mutex->_owner == DFK_THIS_CORO(fiber->dfk));
  CHANGE_STATE(d->state, 5, 6);
  EXPECT_OK(dfk_mutex_unlock(d->mutex))
  EXPECT_OK(dfk_cond_free(d->cv));
  EXPECT_OK(dfk_mutex_free(d->mutex));
}


static void ut_cond_signal_multi_wait_coro2(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  EXPECT(d->mutex->_owner == NULL);
  EXPECT_OK(dfk_cond_signal(d->cv));
  CHANGE_STATE(d->state, 2, 3);
  DFK_POSTPONE_RVOID(fiber->dfk);
  EXPECT_OK(dfk_cond_signal(d->cv));
  CHANGE_STATE(d->state, 4, 5);
}


TOST_F(cond_fixture, cond, signal_multi_wait)
{
  dfk_mutex_t mutex;
  dfk_cond_t cv;
   * Possible values of "state" within this test:
   * 0 - initialized
   * 1 - coro0 waits on cv
   * 2 - coro1 waits on cv
   * 3 - signal submitted
   * 4 - coro0 received signal
   * 5 - signal submitted
   * 6 - coro1 received signal
   *
  int state = 0;
  ut_cond_smw arg = {&mutex, &cv, &state};
  EXPECT(dfk_run(&fixture->dfk, ut_cond_signal_multi_wait_coro0, &arg, 0));
  EXPECT(dfk_run(&fixture->dfk, ut_cond_signal_multi_wait_coro1, &arg, 0));
  EXPECT(dfk_run(&fixture->dfk, ut_cond_signal_multi_wait_coro2, &arg, 0));
  EXPECT_OK(dfk_work(&fixture->dfk));
  EXPECT(state == 6);
}


static void ut_cond_broadcast_multi_wait_coro0(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  EXPECT_OK(dfk_mutex_init(d->mutex, fiber->dfk))
  EXPECT_OK(dfk_cond_init(d->cv, fiber->dfk));
  EXPECT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 0, 1);
  EXPECT_OK(dfk_cond_wait(d->cv, d->mutex));
  CHANGE_STATE(d->state, 3, 4);
  EXPECT(d->mutex->_owner == DFK_THIS_CORO(fiber->dfk));
  EXPECT_OK(dfk_mutex_unlock(d->mutex))
}


static void ut_cond_broadcast_multi_wait_coro1(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  EXPECT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 1, 2);
  EXPECT_OK(dfk_cond_wait(d->cv, d->mutex));
  EXPECT(d->mutex->_owner == DFK_THIS_CORO(fiber->dfk));
  CHANGE_STATE(d->state, 4, 5);
  EXPECT_OK(dfk_mutex_unlock(d->mutex))
  EXPECT_OK(dfk_cond_free(d->cv));
  EXPECT_OK(dfk_mutex_free(d->mutex));
}


static void ut_cond_broadcast_multi_wait_coro2(dfk_fiber_t* fiber, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  DFK_UNUSED(fiber);
  EXPECT(d->mutex->_owner == NULL);
  EXPECT_OK(dfk_cond_broadcast(d->cv));
  CHANGE_STATE(d->state, 2, 3);
}


TOST_F(cond_fixture, cond, broadcast_multi_wait)
{
  dfk_mutex_t mutex;
  dfk_cond_t cv;
   * Possible values of "state" within this test:
   * 0 - initialized
   * 1 - coro0 waits on cv
   * 2 - coro1 waits on cv
   * 3 - broadcast submitted
   * 4 - coro0 received signal
   * 5 - coro1 received signal
  int state = 0;
  ut_cond_smw arg = {&mutex, &cv, &state};
  EXPECT(dfk_run(&fixture->dfk, ut_cond_broadcast_multi_wait_coro0, &arg, 0));
  EXPECT(dfk_run(&fixture->dfk, ut_cond_broadcast_multi_wait_coro1, &arg, 0));
  EXPECT(dfk_run(&fixture->dfk, ut_cond_broadcast_multi_wait_coro2, &arg, 0));
  EXPECT_OK(dfk_work(&fixture->dfk));
  EXPECT(state == 5);
}

*/

