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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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


static void ut_init_free(dfk_coro_t* coro, void* arg)
{
  dfk_mutex_t mutex;
  DFK_UNUSED(arg);
  ASSERT_OK(dfk_mutex_init(&mutex, coro->dfk));
  ASSERT_OK(dfk_mutex_free(&mutex));
}


TEST_F(sync_fixture, mutex, init_free)
{
  ASSERT(dfk_run(&fixture->dfk, ut_init_free, NULL, 0));
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
  }

  if (d->ncoro == 1) {
    ASSERT(dfk_mutex_trylock(d->mutex) == dfk_err_busy);
  }
  dfk_mutex_lock(d->mutex);
  DFK_POSTPONE_RVOID(coro->dfk);
  dfk_mutex_unlock(d->mutex);

  if (d->ncoro == 1) {
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

