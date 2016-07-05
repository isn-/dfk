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

#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/misc.h>
#include "ut.h"


static void ut_check_strtoll(dfk_buf_t buf, long long expected_value, int expected_code)
{
  long long val;
  int ret = dfk_strtoll(buf, NULL, 10, &val);
  ASSERT(ret == expected_code);
  if (ret == dfk_err_ok) {
    ASSERT(val == expected_value);
  }
}


TEST(misc, strtoll_positive)
{
  ut_check_strtoll((dfk_buf_t) {"102301", 6}, 102301, dfk_err_ok);
}


TEST(misc, strtoll_negative)
{
  ut_check_strtoll((dfk_buf_t) {"-9999", 5}, -9999, dfk_err_ok);
}


TEST(misc, strtoll_zero)
{
  ut_check_strtoll((dfk_buf_t) {"0", 1}, 0, dfk_err_ok);
}


TEST(misc, strtoll_overflow)
{
  ut_check_strtoll((dfk_buf_t) {"18446744073709551617", 20}, 0, dfk_err_overflow);
  ut_check_strtoll((dfk_buf_t) {"-18446744073709551616", 21}, 0, dfk_err_overflow);
  ut_check_strtoll((dfk_buf_t) {"-184467440737095516160", 22}, 0, dfk_err_overflow);
}


TEST(misc, strtoll_malformed)
{
  ut_check_strtoll((dfk_buf_t) {"foo", 3}, 0, dfk_err_badarg);
}


TEST(misc, strtoll_partial)
{
  dfk_buf_t nbuf = {"1901-2016", 9};
  char* endptr;
  long long val;
  ASSERT_OK(dfk_strtoll(nbuf, &endptr, 10, &val));
  ASSERT(val == 1901);
  ASSERT(endptr == nbuf.data + 4);
}

