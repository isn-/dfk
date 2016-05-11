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

#include <dfk.h>
#include <dfk/internal.h>
#include "ut.h"


TEST(core, strerror_no_empty_strings)
{
  int i = 0;
  EXPECT(dfk_err_ok == 0);
  for (i = 0; i  < _dfk_err_total; ++i) {
    EXPECT(dfk_strerr(NULL, i));
    EXPECT(strnlen(dfk_strerr(NULL, i), 1))
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
  ASSERT(dfk_run(&dfk, do_inc_arg, &invoked));
  ASSERT_OK(dfk_work(&dfk));
  EXPECT(invoked);
  ASSERT_OK(dfk_free(&dfk));
}


TEST(core, two_coros_in_clip)
{
  dfk_t dfk;
  int invoked = 0;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, do_inc_arg, &invoked));
  ASSERT(dfk_run(&dfk, do_inc_arg, &invoked));
  ASSERT_OK(dfk_work(&dfk));
  EXPECT(invoked == 2);
  ASSERT_OK(dfk_free(&dfk));
}


static void do_spawn_and_die(dfk_coro_t* coro, void* arg)
{
  int* count = (int*) arg;
  *count -= 1;
  if (*count) {
    dfk_run(coro->dfk, do_spawn_and_die, arg);
  }
}


TEST(core, spawn_and_die)
{
  dfk_t dfk;
  int count = 8;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, do_spawn_and_die, &count));
  ASSERT_OK(dfk_work(&dfk));
  EXPECT(count == 0);
  ASSERT_OK(dfk_free(&dfk));
}

