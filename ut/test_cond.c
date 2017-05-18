/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/cond.h>
#include <dfk/internal.h>
#include <dfk/scheduler.h>
#include <ut.h>

#define CHANGE_STATE(state, from, to) \
{ \
  EXPECT((state) == (from)); \
  (state) = (to); \
}

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
  dfk_fiber_name(fiber, "fiber0");
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
  dfk_fiber_name(fiber, "fiber1");
  dfk_t* dfk = fiber->dfk;
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_lock(&f->mutex);
  f->state = 2;
  DFK_POSTPONE(dfk);
  dfk_cond_signal(&f->cv);
  f->state = 3;
  /* fiber 0 should not run because fiber 1 still owns the mutex */
  DFK_POSTPONE(dfk);
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
    DFK_POSTPONE(dfk);
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
  EXPECT(fixture->state == 4);
}

static void wait_unlock_mutex_fiber_0(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber0");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_mutex_lock(&f->mutex);
  CHANGE_STATE(f->state, 0, 1);
  while (f->state != 2) {
    DFK_POSTPONE(dfk);
  }
  CHANGE_STATE(f->state, 2, 3);
  dfk_cond_wait(&f->cv, &f->mutex);
  dfk_mutex_unlock(&f->mutex);
  CHANGE_STATE(f->state, 4, 5);
}

static void wait_unlock_mutex_fiber_1(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber1");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  CHANGE_STATE(f->state, 1, 2);
  dfk_mutex_lock(&f->mutex);
  CHANGE_STATE(f->state, 3, 4);
  dfk_mutex_unlock(&f->mutex);
}

static void wait_unlock_mutex_fiber_2(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber2");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_cond_signal(&f->cv);
}

static void wait_unlock_mutex_fiber_main(dfk_fiber_t* fiber, void* arg)
{
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_mutex_init(&f->mutex, dfk);
  dfk_cond_init(&f->cv, dfk);
  EXPECT(dfk_run(dfk, wait_unlock_mutex_fiber_0, arg, 0));
  while (f->state != 1) {
    DFK_POSTPONE(dfk);
  }
  EXPECT(dfk_run(dfk, wait_unlock_mutex_fiber_1, arg, 0));
  while (f->state != 4) {
    DFK_POSTPONE(dfk);
  }
  EXPECT(dfk_run(dfk, wait_unlock_mutex_fiber_2, arg, 0));
  while (f->state != 5) {
    DFK_POSTPONE(dfk);
  }
  dfk_cond_free(&f->cv);
  dfk_mutex_free(&f->mutex);
}

/*
 * States:
 * 1 - fiber 0 locked mutex
 * 2 - fiber 1 hanged waiting for lock on mutex
 * 3 - fiber 0 released mutex and started waiting for cv
 * 4 - fiber 1 obtained a lock on mutex
 * 5 - fiber 0 received signal, woken up and terminated
 */
TEST_F(cond_fixture, cond, wait_unlock_mutex)
{
  dfk_work(&fixture->dfk, wait_unlock_mutex_fiber_main, fixture, 0);
  EXPECT(fixture->state == 5);
}

static void signal_multi_wait_fiber_0(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber0");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_lock(&f->mutex);
  CHANGE_STATE(f->state, 0, 1);
  dfk_cond_wait(&f->cv, &f->mutex);
  CHANGE_STATE(f->state, 3, 4);
  dfk_mutex_unlock(&f->mutex);
}

static void signal_multi_wait_fiber_1(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber0");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_lock(&f->mutex);
  CHANGE_STATE(f->state, 1, 2);
  dfk_cond_wait(&f->cv, &f->mutex);
  CHANGE_STATE(f->state, 5, 6);
  dfk_mutex_unlock(&f->mutex);
}

static void signal_multi_wait_fiber_2(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber2");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_cond_signal(&f->cv);
  CHANGE_STATE(f->state, 2, 3);
  while (f->state != 4) {
    DFK_POSTPONE(dfk);
  }
  dfk_cond_signal(&f->cv);
  CHANGE_STATE(f->state, 4, 5);
}

static void signal_multi_wait(dfk_fiber_t* fiber, void* arg)
{
  dfk_t* dfk = fiber->dfk;
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_init(&f->mutex, dfk);
  dfk_cond_init(&f->cv, dfk);
  EXPECT(dfk_run(dfk, signal_multi_wait_fiber_0, arg, 0));
  while (f->state != 1) {
    DFK_POSTPONE(dfk);
  }
  EXPECT(dfk_run(dfk, signal_multi_wait_fiber_1, arg, 0));
  while (f->state != 2) {
    DFK_POSTPONE(dfk);
  }
  EXPECT(dfk_run(dfk, signal_multi_wait_fiber_2, arg, 0));
  while (f->state != 6) {
    DFK_POSTPONE(dfk);
  }
  dfk_cond_free(&f->cv);
  dfk_mutex_free(&f->mutex);
}

/*
 * States:
 * 1 - fiber 0 waits on cv
 * 2 - fiber 1 waits on cv
 * 3 - fiber 2 submitted first signal
 * 4 - fiber 0 received signal and unlocked the mutex
 * 5 - fiber 2 submitted second signal
 * 6 - fiber 1 received signal and unlocked the mutex
 */
TEST_F(cond_fixture, cond, signal_multi_wait)
{
  dfk_work(&fixture->dfk, signal_multi_wait, fixture, 0);
  EXPECT(fixture->state == 6);
}

static void broadcast_multi_wait_fiber_0(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber0");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_lock(&f->mutex);
  CHANGE_STATE(f->state, 0, 1);
  dfk_cond_wait(&f->cv, &f->mutex);
  CHANGE_STATE(f->state, 3, 4);
  dfk_mutex_unlock(&f->mutex);
}

static void broadcast_multi_wait_fiber_1(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber1");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_mutex_lock(&f->mutex);
  CHANGE_STATE(f->state, 1, 2);
  dfk_cond_wait(&f->cv, &f->mutex);
  CHANGE_STATE(f->state, 4, 5);
  dfk_mutex_unlock(&f->mutex);
}

static void broadcast_multi_wait_fiber_2(dfk_fiber_t* fiber, void* arg)
{
  dfk_fiber_name(fiber, "fiber2");
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_cond_broadcast(&f->cv);
  CHANGE_STATE(f->state, 2, 3);
}

static void broadcast_multi_wait_fiber_main(dfk_fiber_t* fiber, void* arg)
{
  cond_fixture_t* f = (cond_fixture_t*) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_mutex_init(&f->mutex, dfk);
  dfk_cond_init(&f->cv, dfk);
  EXPECT(dfk_run(dfk, broadcast_multi_wait_fiber_0, arg, 0));
  while (f->state != 1) {
    DFK_POSTPONE(dfk);
  }
  EXPECT(dfk_run(dfk, broadcast_multi_wait_fiber_1, arg, 0));
  while (f->state != 2) {
    DFK_POSTPONE(dfk);
  }
  EXPECT(dfk_run(dfk, broadcast_multi_wait_fiber_2, arg, 0));
  while (f->state != 3) {
    DFK_POSTPONE(dfk);
  }
  while (f->state != 5) {
    DFK_POSTPONE(dfk);
  }
  dfk_cond_free(&f->cv);
  dfk_mutex_free(&f->mutex);
}

/*
 * States:
 * 1 - fiber 0 waits on cv
 * 2 - fiber 1 waits on cv
 * 3 - fiber 2 submitted broadcast
 * 4 - fiber 0 received signal
 * 5 - fiber 1 received signal
 */
TEST_F(cond_fixture, cond, broadcast_multi_wait)
{
  dfk_work(&fixture->dfk, broadcast_multi_wait_fiber_main, fixture, 0);
  EXPECT(fixture->state == 5);
}

