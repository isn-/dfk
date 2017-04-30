/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/context.h>
#include <ut.h>

TEST(context, sizeof)
{
  EXPECT(dfk_sizeof() == sizeof(dfk_t));
}

TEST(context, init_free)
{
  dfk_t dfk;
  dfk_init(&dfk);
  dfk_free(&dfk);
}

TEST(context, default_malloc_realloc_free)
{
  dfk_t dfk;
  dfk_init(&dfk);
  size_t count = 10;
  char* foo = dfk.malloc(&dfk, count * sizeof(char));
  /* check that buffer is accessible */
  for (size_t i = 0; i < count; ++i) {
    foo[i] = (char) i;
  }
  foo = dfk.realloc(&dfk, foo, 2 * count * sizeof(char));
  /* check that buffer is accessible */
  for (size_t i = 0; i < 2 * count; ++i) {
    foo[i] = (char) i + 1;
  }
  dfk.free(&dfk, foo);
}

