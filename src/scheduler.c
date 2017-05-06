/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LISENSE)
 */

#include <dfk/scheduler.h>
#include <dfk/internal.h>
#include <dfk/internal/fiber.h>

void dfk__scheduler_init(dfk_scheduler_t* scheduler, dfk_fiber_t* fiber)
{
  scheduler->fiber = fiber;
  dfk_list_init(&scheduler->pending);
  dfk_list_init(&scheduler->iowait);
  dfk_list_init(&scheduler->terminated);
}

int dfk__scheduler(dfk_scheduler_t* scheduler)
{
  dfk_t* dfk = scheduler->fiber->dfk;

  DFK_DBG(dfk, "fibers pending: %lu, terminated: %lu, iowait: %lu",
      (unsigned long) dfk_list_size(&scheduler->pending),
      (unsigned long) dfk_list_size(&scheduler->terminated),
      (unsigned long) dfk_list_size(&scheduler->iowait));

  if (dfk_list_empty(&scheduler->terminated)
      && dfk_list_empty(&scheduler->pending)
      && dfk_list_empty(&scheduler->iowait)) {
    DFK_DBG(dfk, "{%p} no pending fibers, terminate", (void*) dfk);
    return 1;
  }

  DFK_DBG(dfk, "{%p} cleanup %lu terminated fiber(s)", (void*) dfk,
      (unsigned long) dfk_list_size(&scheduler->terminated));
  while (!dfk_list_empty(&scheduler->terminated)) {
    dfk_list_it begin;
    dfk_list_begin(&scheduler->terminated, &begin);
    dfk_list_hook_t* fiber = begin.value;
    dfk_list_pop_front(&scheduler->terminated);
    DFK_DBG(dfk, "fiber {%p} is terminated, cleanup", (void*) fiber);
    dfk__fiber_free(dfk, (dfk_fiber_t*) fiber);
  }

  DFK_DBG(dfk, "{%p} execute %lu CPU hungry fiber(s)", (void*) dfk,
      (unsigned long) dfk_list_size(&scheduler->pending));
  {
    /*
     * Move scheduler->pending list into a local copy, to prevent iteration over
     * a list that is modified in progress.
     */
    dfk_list_t pending;
    dfk_list_init(&pending);
    dfk_list_move(&scheduler->pending, &pending);
    while (!dfk_list_empty(&pending)) {
      dfk_fiber_t* fiber = (dfk_fiber_t*) dfk_list_front(&pending);
      dfk_list_pop_front(&pending);
      DFK_DBG(dfk, "{%p} next fiber to run {%p}", (void*) dfk, (void*) fiber);
      dfk_yield(scheduler->fiber, fiber);
    }
  }

  if (dfk_list_empty(&scheduler->pending)
      && !dfk_list_empty(&scheduler->iowait)) {
    /*
     * Pending fibers list is empty, while IO hungry fibers
     * exist - switch to IO with possible blocking.
     */
    DFK_DBG(dfk, "no pending fibers, will do I/O");
    dfk_yield(scheduler->fiber, dfk->_eventloop);
  }
  return 0;
}

void dfk__schedule(dfk_t* dfk, dfk_fiber_t* fiber)
{
  assert(dfk);
  assert(fiber);
  /*
   * If the assertion below fails, you're probably trying to call dfk_run()
   * outside of dfk work loop. Fibers can be created only from other fibers.
   * Main fiber is created by the dfk_work() function.
   */
  assert(dfk->_scheduler);
  dfk_list_append(&dfk->_scheduler->pending, &fiber->_hook);
}

void dfk__terminate(dfk_scheduler_t* scheduler, dfk_fiber_t* fiber)
{
  assert(scheduler);
  assert(fiber);
  dfk_list_append(&scheduler->terminated, &fiber->_hook);
  /*
   * Yield back to the scheduler - control will never return here,
   * therefore the line below dfk_yield is marked as unreachable and excluded
   * from lcov coverage report.
   */
  dfk_yield(fiber, scheduler->fiber);
} /* LCOV_EXCL_LINE */

void dfk__scheduler_loop(dfk_fiber_t* fiber, void* arg)
{
  assert(fiber);
  dfk_t* dfk = fiber->dfk;
  assert(dfk);
  dfk_scheduler_t scheduler;
  dfk__scheduler_init(&scheduler, fiber);
  dfk->_scheduler = &scheduler;
  {
    /* Scheduler is initialized - now we can schedule main fiber manually */
    dfk_fiber_t* mainf = (dfk_fiber_t*) arg;
    assert(mainf);
    dfk__schedule(dfk, mainf);
  }
  int ret = 0;
  while (!ret) {
    ret = dfk__scheduler(&scheduler);
    DFK_DBG(dfk, "schedule returned %d, %s", ret,
        ret ? "terminating" : "continue spinning");
  }
  DFK_INFO(dfk, "no pending fibers left in execution queue, job is done");
  dfk->_current = NULL;

  /* Same format as in fiber.c */
#if DFK_NAMED_FIBERS
  DFK_DBG(dfk, "context switch {scheduler} -> {*}");
#else
  DFK_DBG(dfk, "context switch {%p} -> {*}", (void*) dfk->_scheduler);
#endif
  /* Story ends, main character rudes into the sunset */
  coro_transfer(&fiber->_ctx, &dfk->_comeback);
} /* LCOV_EXCL_LINE */

