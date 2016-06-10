/**
 * @copyright
 * Copyright (c) 2015, 2016, Stanislav Ivochkin. All Rights Reserved.
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

#include <stdlib.h>
#include <dfk.h>
#include <dfk/internal.h>
#include "ut.h"


TEST(core, strerror_no_empty_strings)
{
  int i = 0;
  EXPECT(dfk_err_ok == 0);
  for (i = 0; i  < _dfk_err_total; ++i) {
    EXPECT(dfk_strerr(NULL, i));
    EXPECT(strnlen(dfk_strerr(NULL, i), 1));
    EXPECT(strcmp(dfk_strerr(NULL, i), "Unknown error"));
  }
}


TEST(core, strerror_sys_errno)
{
  dfk_t dfk;
  EXPECT(dfk_strerr(NULL, dfk_err_sys));
  EXPECT(strnlen(dfk_strerr(NULL, dfk_err_sys), 1));
  dfk.sys_errno = 10;
  EXPECT(dfk_strerr(&dfk, dfk_err_sys));
  EXPECT(strnlen(dfk_strerr(&dfk, dfk_err_sys), 1));
}


TEST(core, no_run)
{
  dfk_t dfk;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT_OK(dfk_free(&dfk));
}


TEST(core, free_run)
{
  dfk_t dfk;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT_OK(dfk_work(&dfk));
  ASSERT_OK(dfk_free(&dfk));
}


static void do_inc_arg(dfk_coro_t* coro, void* arg)
{
  int* i = (int*) arg;
  DFK_UNUSED(coro);
  *i += 1;
}


TEST(core, no_subroutines)
{
  dfk_t dfk;
  int invoked = 0;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, do_inc_arg, &invoked, 0));
  ASSERT_OK(dfk_work(&dfk));
  EXPECT(invoked);
  ASSERT_OK(dfk_free(&dfk));
}


TEST(core, two_coros_in_clip)
{
  dfk_t dfk;
  int invoked = 0;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, do_inc_arg, &invoked, 0));
  ASSERT(dfk_run(&dfk, do_inc_arg, &invoked, 0));
  ASSERT_OK(dfk_work(&dfk));
  EXPECT(invoked == 2);
  ASSERT_OK(dfk_free(&dfk));
}


static void do_spawn_and_die(dfk_coro_t* coro, void* arg)
{
  int* count = (int*) arg;
  *count -= 1;
  if (*count) {
    dfk_run(coro->dfk, do_spawn_and_die, arg, 0);
  }
}


TEST(core, spawn_and_die)
{
  dfk_t dfk;
  int count = 8;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, do_spawn_and_die, &count, 0));
  ASSERT_OK(dfk_work(&dfk));
  EXPECT(count == 0);
  ASSERT_OK(dfk_free(&dfk));
}


static void* out_of_memory(dfk_t* p, size_t size)
{
  DFK_UNUSED(p);
  DFK_UNUSED(size);
  return NULL;
}


TEST(core, errors)
{
  dfk_t dfk;
  dfk_coro_t coro;
  EXPECT(dfk_init(NULL) == dfk_err_badarg);
  EXPECT(dfk_free(NULL) == dfk_err_badarg);
  EXPECT(dfk_run(NULL, do_spawn_and_die, NULL, 0) == NULL);
  EXPECT(dfk_run(&dfk, NULL, NULL, 0) == NULL);
  dfk_init(&dfk);
  dfk.malloc = out_of_memory;
  EXPECT(dfk_run(&dfk, do_spawn_and_die, NULL, 0) == NULL);
  EXPECT(dfk_coro_name(NULL, "foo") == dfk_err_badarg);
  EXPECT(dfk_coro_name(&coro, NULL, 1) == dfk_err_badarg);
  EXPECT(dfk_yield(NULL, NULL) == dfk_err_badarg);
  EXPECT(dfk_work(NULL) == dfk_err_badarg);
}


TEST(core, default_memory_functions)
{
  dfk_t dfk;
  char* p;
  const size_t size = 10000;
  EXPECT_OK(dfk_init(&dfk));
  p = dfk.malloc(NULL, size);
  ASSERT(p);
  {
    size_t i = 0;
    for (i = 0; i < size; ++i) {
      (void) p[i];
    }
  }
  p = dfk.realloc(NULL, p, 2 * size);
  ASSERT(p);
  {
    size_t i = 0;
    for (i = 0; i < 2 * size; ++i) {
      (void) p[i];
    }
  }
  dfk.free(NULL, p);
}


static void* bad_malloc(dfk_t* dfk, size_t nbytes)
{
  int* nfail = (int*) dfk->userdata;
  if (!*nfail) {
    return NULL;
  }
  (*nfail)--;
  return malloc(nbytes);
}


TEST(core, bad_malloc)
{
  size_t i;
  for (i = 0; i < 2; ++i) {
    dfk_t dfk;
    size_t nfail = i;
    EXPECT_OK(dfk_init(&dfk));
    dfk.userdata = &nfail;
    dfk.malloc = bad_malloc;
    EXPECT(dfk_work(&dfk) == dfk_err_nomem);
    EXPECT_OK(dfk_free(&dfk));
  }
}

