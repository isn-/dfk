/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LISENSE)
 */

#include <dfk/scheduler.h>
#include <dfk/internal.h>

int dfk_scheduler_init(dfk_scheduler_t* scheduler)
{
  DFK_UNUSED(scheduler);
  return 0;
}

int dfk_scheduler_free(dfk_scheduler_t* scheduler)
{
  DFK_UNUSED(scheduler);
  return 0;
}

void dfk_scheduler(dfk_coro_t* scheduler, void* arg)
{
  DFK_UNUSED(p);
  assert(scheduler);
  dfk_t* dfk = scheduler->dfk;
  assert(dfk);
  while (1) {
    DFK_DBG(dfk, "coroutines pending: %lu, terminated: %lu, iowait: %lu",
        (unsigned long) dfk_list_size(&dfk->_pending_coros),
        (unsigned long) dfk_list_size(&dfk->_terminated_coros),
        (unsigned long) dfk_list_size(&dfk->_iowait_coros));

    if (!dfk_list_size(&dfk->_terminated_coros)
        && !dfk_list_size(&dfk->_pending_coros)
        && !dfk_list_size(&dfk->_iowait_coros)) {
      DFK_DBG(dfk, "{%p} no pending coroutines, terminate", (void*) dfk);
      break;
    }

    DFK_DBG(dfk, "{%p} cleanup terminated coroutines", (void*) dfk);
    {
      dfk_coro_t* i = (dfk_coro_t*) dfk->_terminated_coros.head;
      while (i) {
        dfk_coro_t* next = (dfk_coro_t*) i->_hook.next;
        DFK_DBG(dfk, "corotine {%p} is terminated, cleanup", (void*) i);
        dfk__coro_free(i);
        i = next;
      }
      dfk_list_clear(&dfk->_terminated_coros);
    }

    DFK_DBG(dfk, "{%p} execute CPU hungry coroutines", (void*) dfk);
    {
      dfk_coro_t* i = (dfk_coro_t*) dfk->_pending_coros.head;
      while (i) {
        dfk_coro_t* next = (dfk_coro_t*) i->_hook.next;
        DFK_DBG(dfk, "next coroutine to run {%p}", (void*) i);
        DFK_SCHED(dfk, scheduler, i);
        i = next;
      }
      dfk_list_clear(&dfk->_pending_coros);
    }

    if (!dfk_list_size(&dfk->_pending_coros)
        && dfk_list_size(&dfk->_iowait_coros)) {
      /*
       * pending_coros list is empty, while IO hungry coroutines
       * exist - switch to IO with possible blocking
       */
      DFK_DBG(dfk, "no pending coroutines, will do I/O");
      DFK_SCHED(dfk, scheduler, dfk->_eventloop);
    }
  }
  DFK_INFO(dfk, "no pending coroutines left in execution queue, job is done");
  /*
  dfk->_current = NULL;
  dfk_yield(scheduler, NULL);
  */
}


