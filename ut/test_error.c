/**
 * @copyright
 * Copyright (c) 2015-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <string.h>
#include <dfk/error.h>
#include <ut.h>

TEST(strerror, no_empty_strings)
{
  int i = 0;
  EXPECT(dfk_err_ok == 0);
  for (i = 0; i  < _dfk_err_total; ++i) {
    EXPECT(dfk_strerr(NULL, i));
    EXPECT(strlen(dfk_strerr(NULL, i)));
    EXPECT(strcmp(dfk_strerr(NULL, i), "Unknown error"));
  }
}

TEST(strerror, sys_errno)
{
  dfk_t dfk;
  EXPECT(dfk_strerr(NULL, dfk_err_sys));
  EXPECT(strlen(dfk_strerr(NULL, dfk_err_sys)));
  dfk.sys_errno = 10;
  EXPECT(dfk_strerr(&dfk, dfk_err_sys));
  EXPECT(strlen(dfk_strerr(&dfk, dfk_err_sys)));
}

TEST(strerror, unknown_error)
{
  EXPECT(!strcmp(dfk_strerr(NULL, -1), "Unknown error"));
}

