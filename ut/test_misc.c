/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
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

