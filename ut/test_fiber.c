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

static void do_inc_arg(dfk_fiber_t* fiber, void* arg)
{
  int* i = (int*) arg;
  DFK_UNUSED(fiber);
  *i += 1;
}

TEST(fiber, sizeof)
{
  EXPECT(dfk_fiber_sizeof() == sizeof(dfk_fiber_t));
}

TEST(fiber, run_one)
{
  dfk_t dfk;
  int invoked = 15;
  dfk_init(&dfk);
  EXPECT_OK(dfk_work(&dfk, do_inc_arg, &invoked, 0));
  EXPECT(invoked == 16);
  dfk_free(&dfk);
}

static void spawn_child_main(dfk_fiber_t* fiber, void* arg)
{
  int* spawned = (int*) arg;
  (*spawned)++;
  if (*spawned == 1) {
    /* Only first fiber should spawn a child */
    dfk_spawn(fiber->dfk, spawn_child_main, arg, 0);
  }
}

TEST(fiber, spawn_child)
{
  dfk_t dfk;
  dfk_init(&dfk);
  int spawned = 0;
  dfk_work(&dfk, spawn_child_main, &spawned, 0);
  dfk_free(&dfk);
}

static void copy_arg_main(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(fiber);
  int* iarg = arg;
  EXPECT(*iarg == 200);
  *iarg = 100;
}

TEST(fiber, copy_arg)
{
  dfk_t dfk;
  dfk_init(&dfk);
  int arg = 200;
  dfk_work(&dfk, copy_arg_main, &arg, sizeof(arg));
  EXPECT(arg == 200);
  dfk_free(&dfk);
}

static void noop_fiber(dfk_fiber_t* fiber, void* arg)
{
  DFK_UNUSED(fiber);
  DFK_UNUSED(arg);
}

TEST(fiber, no_need_to_align_stack)
{
  dfk_t dfk;
  dfk_init(&dfk);
  char arg[DFK_STACK_ALIGNMENT - (sizeof(dfk_fiber_t) % DFK_STACK_ALIGNMENT)];
  dfk_work(&dfk, noop_fiber, arg, sizeof(arg));
  dfk_free(&dfk);
}

static void spawn_child_oom_main(dfk_fiber_t* fiber, void* arg)
{
  EXPECT(!dfk_spawn(fiber->dfk, noop_fiber, arg, 0));
}

TEST(fiber, spawn_child_oom)
{
  dfk_t dfk;
  dfk_init(&dfk);
  int spawned = 0;
  size_t nsuccessfulallocs = 3;
  dfk.user.data = &nsuccessfulallocs;
  dfk.malloc = malloc_first_n_ok;
  dfk_work(&dfk, spawn_child_oom_main, &spawned, 0);
  dfk_free(&dfk);
}

