/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/context.h>
#include <dfk/fiber.h>
#include <dfk/internal.h>
#include <ut.h>
#include <allocators.h>

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

static void empty_fiber(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(fiber);
  DFK_UNUSED(arg);
}

/*
 * Checks that dfk_work tolerates out-of-memory and performs at most
 * max_allocs memory allocations.
 */
TEST(context, work_out_of_memory)
{
  dfk_t dfk;
  dfk_init(&dfk);
  dfk.malloc = malloc_first_n_ok;
  size_t max_allocs = 16;
  for (size_t i = 0; i < max_allocs; ++i) {
    size_t nallocs = i;
    dfk.user.data = &nallocs;
    int ret = dfk_work(&dfk, empty_fiber, NULL, 0);
    if (ret == dfk_err_ok) {
      break;
    }
    EXPECT(ret == dfk_err_nomem);
  }
}

