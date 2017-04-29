/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/misc.h>
#include <ut.h>


TEST(buf, sizeof)
{
  EXPECT(dfk_buf_sizeof() == sizeof(dfk_buf_t));
}


TEST(iovec, sizeof)
{
  EXPECT(dfk_iovec_sizeof() == sizeof(dfk_iovec_t));
}


TEST(buf, append_ok)
{
  char c[] = "Hello world";
  dfk_buf_t buf = {c, 4};
  dfk_buf_append(&buf, c + 4, sizeof(c) - 4);
  EXPECT(buf.size == sizeof(c));
}


TEST(buf, append_null)
{
  char c[] = "Hello world";
  dfk_buf_t buf = {NULL, 0};
  dfk_buf_append(&buf, c, sizeof(c) - 1);
  EXPECT(buf.data == c);
  EXPECT(buf.size == sizeof(c) - 1);
}

