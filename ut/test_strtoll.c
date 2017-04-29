/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/error.h>
#include <dfk/strtoll.h>
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

TEST(strtoll, positive)
{
  ut_check_strtoll((dfk_buf_t) {"102301", 6}, 102301, dfk_err_ok);
}

TEST(strtoll, negative)
{
  ut_check_strtoll((dfk_buf_t) {"-9999", 5}, -9999, dfk_err_ok);
}

TEST(strtoll, zero)
{
  ut_check_strtoll((dfk_buf_t) {"0", 1}, 0, dfk_err_ok);
}

TEST(strtoll, overflow)
{
  ut_check_strtoll((dfk_buf_t) {"18446744073709551617", 20}, 0, dfk_err_overflow);
  ut_check_strtoll((dfk_buf_t) {"-18446744073709551616", 21}, 0, dfk_err_overflow);
  ut_check_strtoll((dfk_buf_t) {"-184467440737095516160", 22}, 0, dfk_err_overflow);
}

TEST(strtoll, malformed)
{
  ut_check_strtoll((dfk_buf_t) {"foo", 3}, 0, dfk_err_badarg);
}

TEST(strtoll, partial)
{
  dfk_buf_t nbuf = {"1901-2016", 9};
  char* endptr;
  long long val;
  EXPECT_OK(dfk_strtoll(nbuf, &endptr, 10, &val));
  EXPECT(val == 1901);
  EXPECT(endptr == nbuf.data + 4);
}

TEST(strtoll, bad_base)
{
  long long val;
  int ret = dfk_strtoll((dfk_buf_t) {"1000", 4}, NULL, 37, &val);
  EXPECT(ret == dfk_err_badarg);
}

