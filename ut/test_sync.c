/**
 * @copyright
 * Copyright (c) 2016, Stanislav Ivochkin. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <assert.h>
#include <dfk.h>
#include <dfk/internal.h>
#include "ut.h"


typedef struct {
  dfk_t dfk;
} sync_fixture_t;


static void sync_fixture_setup(sync_fixture_t* f)
{
  dfk_init(&f->dfk);
}


static void sync_fixture_teardown(sync_fixture_t* f)
{
  dfk_free(&f->dfk);
}


static void ut_mutex_init_free_coro(dfk_coro_t* coro, void* arg)
{
  dfk_mutex_t mutex;
  DFK_UNUSED(arg);
  ASSERT_OK(dfk_mutex_init(&mutex, coro->dfk));
  ASSERT_OK(dfk_mutex_free(&mutex));
}


TEST_F(sync_fixture, mutex, init_free)
{
  ASSERT(dfk_run(&fixture->dfk, ut_mutex_init_free_coro, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_recursive_lock(dfk_coro_t* coro, void* arg)
{
  dfk_mutex_t mutex;
  DFK_UNUSED(arg);
  ASSERT_OK(dfk_mutex_init(&mutex, coro->dfk));
  ASSERT_OK(dfk_mutex_lock(&mutex));
  ASSERT_OK(dfk_mutex_lock(&mutex));
  ASSERT_OK(dfk_mutex_unlock(&mutex));
  ASSERT_OK(dfk_mutex_free(&mutex));
}


TEST_F(sync_fixture, mutex, recursive_lock)
{
  ASSERT(dfk_run(&fixture->dfk, ut_recursive_lock, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_recursive_trylock(dfk_coro_t* coro, void* arg)
{
  dfk_mutex_t mutex;
  DFK_UNUSED(arg);
  ASSERT_OK(dfk_mutex_init(&mutex, coro->dfk));
  ASSERT_OK(dfk_mutex_trylock(&mutex));
  ASSERT_OK(dfk_mutex_trylock(&mutex));
  ASSERT_OK(dfk_mutex_unlock(&mutex));
  ASSERT_OK(dfk_mutex_free(&mutex));
}


TEST_F(sync_fixture, mutex, recursive_trylock)
{
  ASSERT(dfk_run(&fixture->dfk, ut_recursive_trylock, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


typedef struct ut_two_coros_contention_data {
  dfk_mutex_t* mutex;
  int ncoro;
  int* counter;
} ut_two_coros_contention_data;


static void ut_two_coros_contention_coro(dfk_coro_t* coro, void* arg)
{
  ut_two_coros_contention_data* d = (ut_two_coros_contention_data*) arg;
  if (d->ncoro == 0) {
    ASSERT_OK(dfk_mutex_init(d->mutex, coro->dfk));
  }

  ASSERT(*d->counter == d->ncoro);
  (*d->counter)++;
  dfk_mutex_lock(d->mutex);
  DFK_POSTPONE_RVOID(coro->dfk);
  ASSERT(*d->counter == d->ncoro + 2);
  (*d->counter)++;
  dfk_mutex_unlock(d->mutex);
  ASSERT(*d->counter == d->ncoro + 3);

  if (d->ncoro == 1) {
    ASSERT_OK(dfk_mutex_free(d->mutex));
  }
}


TEST_F(sync_fixture, mutex, two_coros_contention)
{
  dfk_mutex_t mutex;
  int counter = 0;
  ut_two_coros_contention_data data[] = {
    {&mutex, 0, &counter},
    {&mutex, 1, &counter}
  };
  ASSERT(dfk_run(&fixture->dfk, ut_two_coros_contention_coro, &data[0], 0));
  ASSERT(dfk_run(&fixture->dfk, ut_two_coros_contention_coro, &data[1], 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


typedef struct ut_try_lock_data {
  dfk_mutex_t* mutex;
  int ncoro;
} ut_try_lock_data;


static void ut_try_lock(dfk_coro_t* coro, void* arg)
{
  ut_try_lock_data* d = (ut_try_lock_data*) arg;
  if (d->ncoro == 0) {
    ASSERT_OK(dfk_mutex_init(d->mutex, coro->dfk));
    ASSERT_OK(dfk_mutex_trylock(d->mutex));
    DFK_POSTPONE_RVOID(coro->dfk);
    ASSERT_OK(dfk_mutex_unlock(d->mutex));
  } else {
    assert(d->ncoro == 1);
    ASSERT(dfk_mutex_trylock(d->mutex) == dfk_err_busy);
    ASSERT_OK(dfk_mutex_lock(d->mutex));
    ASSERT_OK(dfk_mutex_unlock(d->mutex));
    ASSERT_OK(dfk_mutex_free(d->mutex));
  }
}


TEST_F(sync_fixture, mutex, try_lock)
{
  dfk_mutex_t mutex;
  ut_try_lock_data data[] = {
    {&mutex, 0},
    {&mutex, 1}
  };
  ASSERT(dfk_run(&fixture->dfk, ut_try_lock, &data[0], 0));
  ASSERT(dfk_run(&fixture->dfk, ut_try_lock, &data[1], 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_mutex_errors_coro(dfk_coro_t* coro, void* arg)
{
  dfk_mutex_t mutex;
  DFK_UNUSED(arg);
  ASSERT(dfk_mutex_init(NULL, NULL) == dfk_err_badarg);
  ASSERT(dfk_mutex_init(NULL, coro->dfk) == dfk_err_badarg);
  ASSERT(dfk_mutex_init(&mutex, NULL) == dfk_err_badarg);
  ASSERT_OK(dfk_mutex_init(&mutex, coro->dfk));
  ASSERT(dfk_mutex_free(NULL) == dfk_err_badarg);
  ASSERT_OK(dfk_mutex_lock(&mutex));
  ASSERT(dfk_mutex_free(&mutex) == dfk_err_busy);
  ASSERT_OK(dfk_mutex_unlock(&mutex));
  ASSERT(dfk_mutex_lock(NULL) == dfk_err_badarg);
  ASSERT(dfk_mutex_unlock(NULL) == dfk_err_badarg);
  ASSERT(dfk_mutex_trylock(NULL) == dfk_err_badarg);
}


TEST_F(sync_fixture, mutex, errors)
{
  ASSERT(dfk_run(&fixture->dfk, ut_mutex_errors_coro, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_cond_init_free_coro(dfk_coro_t* coro, void* arg)
{
  dfk_cond_t cv;
  DFK_UNUSED(arg);
  ASSERT_OK(dfk_cond_init(&cv, coro->dfk));
  ASSERT_OK(dfk_cond_free(&cv));
}


TEST_F(sync_fixture, cond, init_free)
{
  ASSERT(dfk_run(&fixture->dfk, ut_cond_init_free_coro, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_cond_signal_empty_waitqueue_coro(dfk_coro_t* coro, void* arg)
{
  dfk_cond_t cv;
  DFK_UNUSED(arg);
  ASSERT_OK(dfk_cond_init(&cv, coro->dfk));
  ASSERT_OK(dfk_cond_signal(&cv));
  ASSERT_OK(dfk_cond_free(&cv));
}


TEST_F(sync_fixture, cond, signal_empty_waitqueue)
{
  ASSERT(dfk_run(&fixture->dfk, ut_cond_signal_empty_waitqueue_coro, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_cond_broadcast_empty_waitqueue_coro(dfk_coro_t* coro, void* arg)
{
  dfk_cond_t cv;
  DFK_UNUSED(arg);
  ASSERT_OK(dfk_cond_init(&cv, coro->dfk));
  ASSERT_OK(dfk_cond_broadcast(&cv));
  ASSERT_OK(dfk_cond_free(&cv));
}


TEST_F(sync_fixture, cond, broadcast_empty_waitqueue)
{
  ASSERT(dfk_run(&fixture->dfk, ut_cond_broadcast_empty_waitqueue_coro, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_cond_errors_coro(dfk_coro_t* coro, void* arg)
{
  dfk_cond_t cond;
  dfk_mutex_t mutex;
  DFK_UNUSED(arg);
  ASSERT(dfk_cond_init(NULL, NULL) == dfk_err_badarg);
  ASSERT(dfk_cond_init(NULL, coro->dfk) == dfk_err_badarg);
  ASSERT(dfk_cond_init(&cond, NULL) == dfk_err_badarg);
  ASSERT_OK(dfk_cond_init(&cond, coro->dfk));
  ASSERT(dfk_cond_free(NULL) == dfk_err_badarg);
  ASSERT_OK(dfk_mutex_init(&mutex, coro->dfk));
  ASSERT(dfk_cond_wait(NULL, NULL) == dfk_err_badarg);
  ASSERT(dfk_cond_wait(&cond, NULL) == dfk_err_badarg);
  ASSERT(dfk_cond_wait(NULL, &mutex) == dfk_err_badarg);
  ASSERT(dfk_cond_signal(NULL) == dfk_err_badarg);
  ASSERT(dfk_cond_broadcast(NULL) == dfk_err_badarg);
  ASSERT_OK(dfk_cond_free(&cond));
}


TEST_F(sync_fixture, cond, errors)
{
  ASSERT(dfk_run(&fixture->dfk, ut_cond_errors_coro, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_cond_free_busy_coro0(dfk_coro_t* coro, void* arg)
{
  dfk_cond_t* cv = (dfk_cond_t*) arg;
  dfk_mutex_t mutex;
  ASSERT_OK(dfk_cond_init(cv, coro->dfk));
  ASSERT_OK(dfk_mutex_init(&mutex, coro->dfk));
  ASSERT_OK(dfk_mutex_lock(&mutex));
  ASSERT_OK(dfk_cond_wait(cv, &mutex));
  ASSERT_OK(dfk_mutex_unlock(&mutex));
  ASSERT_OK(dfk_mutex_free(&mutex));
  ASSERT_OK(dfk_cond_free(cv));
}


static void ut_cond_free_busy_coro1(dfk_coro_t* coro, void* arg)
{
  dfk_cond_t* cv = (dfk_cond_t*) arg;
  DFK_UNUSED(coro);
  ASSERT(dfk_cond_free(cv) == dfk_err_busy);
  ASSERT_OK(dfk_cond_signal(cv));
}


TEST_F(sync_fixture, cond, free_busy)
{
  dfk_cond_t cv;
  ASSERT(dfk_run(&fixture->dfk, ut_cond_free_busy_coro0, &cv, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_free_busy_coro1, &cv, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


typedef struct ut_cond_ssw {
  dfk_mutex_t* mutex;
  dfk_cond_t* cv;
} ut_cond_ssw;


static void ut_cond_signal_single_wait_coro0(dfk_coro_t* coro, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  ASSERT_OK(dfk_mutex_init(d->mutex, coro->dfk))
  ASSERT_OK(dfk_cond_init(d->cv, coro->dfk));
  ASSERT_OK(dfk_mutex_lock(d->mutex))
  ASSERT_OK(dfk_cond_wait(d->cv, d->mutex));
  ASSERT(d->mutex->_.owner);
  ASSERT_OK(dfk_mutex_unlock(d->mutex))
  ASSERT_OK(dfk_cond_free(d->cv));
  ASSERT_OK(dfk_mutex_free(d->mutex));
}


static void ut_cond_signal_single_wait_coro1(dfk_coro_t* coro, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  ASSERT(d->mutex->_.owner == NULL);
  ASSERT_OK(dfk_cond_signal(d->cv));
  DFK_UNUSED(coro);
}


TEST_F(sync_fixture, cond, signal_single_wait)
{
  dfk_mutex_t mutex;
  dfk_cond_t cv;
  ut_cond_ssw arg = {&mutex, &cv};
  ASSERT(dfk_run(&fixture->dfk, ut_cond_signal_single_wait_coro0, &arg, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_signal_single_wait_coro1, &arg, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void ut_cond_wait_unlock_mutex_coro0(dfk_coro_t* coro, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  ASSERT_OK(dfk_mutex_init(d->mutex, coro->dfk));
  ASSERT_OK(dfk_cond_init(d->cv, coro->dfk));
  ASSERT_OK(dfk_mutex_lock(d->mutex));
  DFK_POSTPONE_RVOID(coro->dfk);
  ASSERT_OK(dfk_cond_wait(d->cv, d->mutex));
  ASSERT_OK(dfk_mutex_unlock(d->mutex));
  ASSERT_OK(dfk_cond_free(d->cv));
  ASSERT_OK(dfk_mutex_free(d->mutex));
}


static void ut_cond_wait_unlock_mutex_coro1(dfk_coro_t* coro, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  DFK_UNUSED(coro);
  ASSERT_OK(dfk_mutex_lock(d->mutex));
  ASSERT_OK(dfk_mutex_unlock(d->mutex));
}


static void ut_cond_wait_unlock_mutex_coro2(dfk_coro_t* coro, void* arg)
{
  ut_cond_ssw* d = (ut_cond_ssw*) arg;
  DFK_POSTPONE_RVOID(coro->dfk);
  DFK_POSTPONE_RVOID(coro->dfk);
  ASSERT_OK(dfk_cond_signal(d->cv));
}


TEST_F(sync_fixture, cond, wait_unlock_mutex)
{
  dfk_mutex_t mutex;
  dfk_cond_t cv;
  ut_cond_ssw arg = {&mutex, &cv};
  ASSERT(dfk_run(&fixture->dfk, ut_cond_wait_unlock_mutex_coro0, &arg, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_wait_unlock_mutex_coro1, &arg, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_wait_unlock_mutex_coro2, &arg, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


typedef struct ut_cond_smw {
  dfk_mutex_t* mutex;
  dfk_cond_t* cv;
  int* state;
} ut_cond_smw;


#define CHANGE_STATE(pstate, from, to) \
{ \
  ASSERT(*(pstate) == (from)); \
  *(pstate) = (to); \
}


static void ut_cond_signal_multi_wait_coro0(dfk_coro_t* coro, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  ASSERT_OK(dfk_mutex_init(d->mutex, coro->dfk))
  ASSERT_OK(dfk_cond_init(d->cv, coro->dfk));
  ASSERT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 0, 1);
  ASSERT_OK(dfk_cond_wait(d->cv, d->mutex));
  CHANGE_STATE(d->state, 3, 4);
  ASSERT(d->mutex->_.owner == DFK_THIS_CORO(coro->dfk));
  ASSERT_OK(dfk_mutex_unlock(d->mutex))
}


static void ut_cond_signal_multi_wait_coro1(dfk_coro_t* coro, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  ASSERT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 1, 2);
  ASSERT_OK(dfk_cond_wait(d->cv, d->mutex));
  ASSERT(d->mutex->_.owner == DFK_THIS_CORO(coro->dfk));
  CHANGE_STATE(d->state, 5, 6);
  ASSERT_OK(dfk_mutex_unlock(d->mutex))
  ASSERT_OK(dfk_cond_free(d->cv));
  ASSERT_OK(dfk_mutex_free(d->mutex));
}


static void ut_cond_signal_multi_wait_coro2(dfk_coro_t* coro, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  ASSERT(d->mutex->_.owner == NULL);
  ASSERT_OK(dfk_cond_signal(d->cv));
  CHANGE_STATE(d->state, 2, 3);
  DFK_POSTPONE_RVOID(coro->dfk);
  ASSERT_OK(dfk_cond_signal(d->cv));
  CHANGE_STATE(d->state, 4, 5);
}


TEST_F(sync_fixture, cond, signal_multi_wait)
{
  dfk_mutex_t mutex;
  dfk_cond_t cv;
  /**
   * Possible values of "state" within this test:
   * 0 - initialized
   * 1 - coro0 waits on cv
   * 2 - coro1 waits on cv
   * 3 - signal submitted
   * 4 - coro0 received signal
   * 5 - signal submitted
   * 6 - coro1 received signal
   */
  int state = 0;
  ut_cond_smw arg = {&mutex, &cv, &state};
  ASSERT(dfk_run(&fixture->dfk, ut_cond_signal_multi_wait_coro0, &arg, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_signal_multi_wait_coro1, &arg, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_signal_multi_wait_coro2, &arg, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
  ASSERT(state == 6);
}


static void ut_cond_broadcast_multi_wait_coro0(dfk_coro_t* coro, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  ASSERT_OK(dfk_mutex_init(d->mutex, coro->dfk))
  ASSERT_OK(dfk_cond_init(d->cv, coro->dfk));
  ASSERT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 0, 1);
  ASSERT_OK(dfk_cond_wait(d->cv, d->mutex));
  CHANGE_STATE(d->state, 3, 4);
  ASSERT(d->mutex->_.owner == DFK_THIS_CORO(coro->dfk));
  ASSERT_OK(dfk_mutex_unlock(d->mutex))
}


static void ut_cond_broadcast_multi_wait_coro1(dfk_coro_t* coro, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  ASSERT_OK(dfk_mutex_lock(d->mutex));
  CHANGE_STATE(d->state, 1, 2);
  ASSERT_OK(dfk_cond_wait(d->cv, d->mutex));
  ASSERT(d->mutex->_.owner == DFK_THIS_CORO(coro->dfk));
  CHANGE_STATE(d->state, 4, 5);
  ASSERT_OK(dfk_mutex_unlock(d->mutex))
  ASSERT_OK(dfk_cond_free(d->cv));
  ASSERT_OK(dfk_mutex_free(d->mutex));
}


static void ut_cond_broadcast_multi_wait_coro2(dfk_coro_t* coro, void* arg)
{
  ut_cond_smw* d = (ut_cond_smw*) arg;
  DFK_UNUSED(coro);
  ASSERT(d->mutex->_.owner == NULL);
  ASSERT_OK(dfk_cond_broadcast(d->cv));
  CHANGE_STATE(d->state, 2, 3);
}


TEST_F(sync_fixture, cond, broadcast_multi_wait)
{
  dfk_mutex_t mutex;
  dfk_cond_t cv;
  /**
   * Possible values of "state" within this test:
   * 0 - initialized
   * 1 - coro0 waits on cv
   * 2 - coro1 waits on cv
   * 3 - broadcast submitted
   * 4 - coro0 received signal
   * 5 - coro1 received signal
   */
  int state = 0;
  ut_cond_smw arg = {&mutex, &cv, &state};
  ASSERT(dfk_run(&fixture->dfk, ut_cond_broadcast_multi_wait_coro0, &arg, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_broadcast_multi_wait_coro1, &arg, 0));
  ASSERT(dfk_run(&fixture->dfk, ut_cond_broadcast_multi_wait_coro2, &arg, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
  ASSERT(state == 5);
}

