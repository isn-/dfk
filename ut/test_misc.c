/**
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/misc.h>
#include <ut.h>


static void ut_check_strtoll(dfk_buf_t buf, long long expected_value, int expected_code)
{
  long long val;
  int ret = dfk_strtoll(buf, NULL, 10, &val);
  EXPECT(ret == expected_code);
  if (ret == dfk_err_ok) {
    EXPECT(val == expected_value);
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
  EXPECT_OK(dfk_strtoll(nbuf, &endptr, 10, &val));
  EXPECT(val == 1901);
  EXPECT(endptr == nbuf.data + 4);
}


TEST(misc, buf_append_ok)
{
  char c[] = "Hello world";
  dfk_buf_t buf = {c, 4};
  dfk_buf_append(&buf, c + 4, sizeof(c) - 4);
  EXPECT(buf.size == sizeof(c));
}


TEST(misc, buf_append_null)
{
  char c[] = "Hello world";
  dfk_buf_t buf = {NULL, 0};
  dfk_buf_append(&buf, c, sizeof(c) - 1);
  EXPECT(buf.data == c);
  EXPECT(buf.size == sizeof(c) - 1);
}

