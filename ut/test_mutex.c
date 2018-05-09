/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/mutex.h>
#include <dfk/internal.h>
#include <dfk/scheduler.h>
#include <ut.h>

typedef struct {
  dfk_t dfk;
} mutex_fixture_t;

static void mutex_fixture_setup(mutex_fixture_t* f)
{
  dfk_init(&f->dfk);
}

static void mutex_fixture_teardown(mutex_fixture_t* f)
{
  dfk_free(&f->dfk);
}

static void mutex_init_free(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(arg);
  dfk_mutex_t mutex;
  dfk_mutex_init(&mutex, fiber->dfk);
  dfk_mutex_free(&mutex);
}

TEST_F(mutex_fixture, mutex, init_free)
{
  EXPECT_OK(dfk_work(&fixture->dfk, mutex_init_free, NULL, 0));
}

static void recursive_lock(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(arg);
  dfk_mutex_t mutex;
  dfk_mutex_init(&mutex, fiber->dfk);
  dfk_mutex_lock(&mutex);
  dfk_mutex_lock(&mutex);
  dfk_mutex_unlock(&mutex);
  dfk_mutex_free(&mutex);
}

TEST_F(mutex_fixture, mutex, recursive_lock)
{
  EXPECT_OK(dfk_work(&fixture->dfk, recursive_lock, NULL, 0));
}

static void recursive_trylock(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(arg);
  dfk_mutex_t mutex;
  dfk_mutex_init(&mutex, fiber->dfk);
  EXPECT_OK(dfk_mutex_trylock(&mutex));
  EXPECT_OK(dfk_mutex_trylock(&mutex));
  dfk_mutex_unlock(&mutex);
  dfk_mutex_free(&mutex);
}

TEST_F(mutex_fixture, mutex, recursive_trylock)
{
  EXPECT_OK(dfk_work(&fixture->dfk, recursive_trylock, NULL, 0));
}

typedef struct two_fibers_contention_data {
  dfk_mutex_t* mutex;
  int nfiber;
  int* counter;
} two_fibers_contention_data;

static void two_fibers_contention_worker(dfk_fiber_t* fiber, void* arg)
{
  dfk_t* dfk = fiber->dfk;
  two_fibers_contention_data* d = (two_fibers_contention_data*) arg;
  /*
   * Fiber 1 waits until fiber 0 acquires the mutex. Before acquiring mutex,
   * fiber 0 increments counter.
   */
  if (*d->counter == 0 && d->nfiber == 1) {
    DFK_POSTPONE(dfk);
  }
  /*
   * When fiber 0 reaches this line, counter is equal to 0. When fiber 1 reaches
   * this line, counter is already incremented by fiber 0 and is equal to 1.
   */
  EXPECT(*d->counter == d->nfiber);
  (*d->counter)++;
  /* Fiber 0 acquires mutex first */
  dfk_mutex_lock(d->mutex);
  /*
   * Suspend at this point should have no effect for fibers ordering - mutex
   * shouldn't be acquired by fiber 1 until it is released by fiber 0. The goal
   * of the suspend here is for letting fiber 1 to increment counter before
   * locking on dfk_mutex_lock in the line above.
   */
  DFK_POSTPONE(fiber->dfk);
  EXPECT(*d->counter == d->nfiber + 2);
  (*d->counter)++;
  /* Should return to the same fiber */
  DFK_POSTPONE(fiber->dfk);
  EXPECT(*d->counter == d->nfiber + 3);
  dfk_mutex_unlock(d->mutex);

  if (d->nfiber == 1) {
    dfk_mutex_free(d->mutex);
  }
}

static void two_fibers_contention_main(dfk_fiber_t* fiber, void* arg)
{
  dfk_t* dfk = fiber->dfk;
  two_fibers_contention_data* data = arg;
  dfk_fiber_name(fiber, "utmain");
  dfk_mutex_init(data->mutex, dfk);
  dfk_fiber_t* w1 = dfk_spawn(dfk, two_fibers_contention_worker, data, 0);
  dfk_fiber_t* w2 = dfk_spawn(dfk, two_fibers_contention_worker, data + 1, 0);
  dfk_fiber_name(w1, "worker1");
  dfk_fiber_name(w2, "worker2");
}

TEST_F(mutex_fixture, mutex, two_fibers_contention)
{
  dfk_mutex_t mutex;
  int counter = 0;
  two_fibers_contention_data data[] = {
    {&mutex, 0, &counter},
    {&mutex, 1, &counter}
  };
  EXPECT_OK(dfk_work(&fixture->dfk, two_fibers_contention_main, data, 0));
  EXPECT(counter == 4);
}

typedef struct try_lock_data {
  dfk_mutex_t* mutex;
  int nfiber;
} try_lock_data;

static void try_lock_worker(dfk_fiber_t* fiber, void* arg)
{
  dfk_t* dfk = fiber->dfk;
  try_lock_data* d = arg;
  if (d->nfiber == 0) {
    dfk_mutex_init(d->mutex, dfk);
    EXPECT_OK(dfk_mutex_trylock(d->mutex));
    DFK_POSTPONE(dfk);
    dfk_mutex_unlock(d->mutex);
  } else {
    assert(d->nfiber == 1);
    EXPECT(dfk_mutex_trylock(d->mutex) == dfk_err_busy);
    dfk_mutex_lock(d->mutex);
    dfk_mutex_unlock(d->mutex);
    dfk_mutex_free(d->mutex);
  }
}

static void try_lock_main(dfk_fiber_t* fiber, void* arg)
{
  dfk_t* dfk = fiber->dfk;
  try_lock_data* data = arg;
  EXPECT(dfk_spawn(dfk, try_lock_worker, data, 0));
  EXPECT(dfk_spawn(dfk, try_lock_worker, data + 1, 0));
}

TEST_F(mutex_fixture, mutex, try_lock)
{
  dfk_mutex_t mutex;
  try_lock_data data[] = {
    {&mutex, 0},
    {&mutex, 1}
  };
  EXPECT_OK(dfk_work(&fixture->dfk, try_lock_main, data, 0));
}

