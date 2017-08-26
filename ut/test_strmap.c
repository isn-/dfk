/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/context.h>
#include <dfk/strmap.h>
#include <dfk/arena.h>
#include <ut.h>

typedef struct fixture_t {
  dfk_t dfk;
  dfk_arena_t arena;
  dfk_strmap_t map;
} fixture_t;

static void fixture_setup(fixture_t* f)
{
  dfk_init(&f->dfk);
  dfk_arena_init(&f->arena, &f->dfk);
  dfk_strmap_init(&f->map);
}

static void fixture_teardown(fixture_t* f)
{
  dfk_arena_free(&f->arena);
  dfk_free(&f->dfk);
}

TEST_F(fixture, strmap, init_free)
{
  EXPECT(fixture);
}

