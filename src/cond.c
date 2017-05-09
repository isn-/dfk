/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <dfk/cond.h>
#include <dfk/internal.h>
#include <dfk/scheduler.h>

#define TO_FIBER(expr) DFK_CONTAINER_OF((expr), dfk_fiber_t, _hook)

void dfk_cond_init(dfk_cond_t* cond, dfk_t* dfk)
{
  assert(cond);
  assert(dfk);
  DFK_DBG(dfk, "{%p}", (void*) cond);
  dfk_list_init(&cond->_waitqueue);
  cond->dfk = dfk;
}

void dfk_cond_free(dfk_cond_t* cond)
{
  assert(cond);
  DFK_DBG(cond->dfk, "{%p}", (void*) cond);
  /* Attempt to free non-empty condition variable */
  assert(dfk_list_empty(&cond->_waitqueue));
}

void dfk_cond_wait(dfk_cond_t* cond, dfk_mutex_t* mutex)
{
  assert(cond);
  assert(mutex);
  dfk_t* dfk = cond->dfk;
  dfk_fiber_t* this = dfk__this_fiber(dfk->_scheduler);
  DFK_DBG(dfk, "{%p}", (void*) cond);
  assert(dfk == mutex->dfk);
  assert(mutex->_owner);
  assert(mutex->_owner == this);
  if (dfk_list_empty(&mutex->_waitqueue)) {
    DFK_DBG(mutex->dfk, "{%p} is now unlocked, no fiber is waiting for lock",
        (void*) mutex);
    mutex->_owner = NULL;
  } else {
    dfk_fiber_t* next = TO_FIBER(dfk_list_front(&mutex->_waitqueue));
    dfk_list_pop_front(&mutex->_waitqueue);
    DFK_DBG(mutex->dfk, "{%p} is acquired by {%p}, wait queue size %llu",
        (void*) mutex, (void*) next,
        (unsigned long long) dfk_list_size(&mutex->_waitqueue));
    mutex->_owner = next;
    dfk__resume(dfk->_scheduler, next);
  }
  dfk_list_append(&cond->_waitqueue, &this->_hook);
  dfk__suspend(dfk->_scheduler);
  dfk_mutex_lock(mutex);
}

void dfk_cond_signal(dfk_cond_t* cond)
{
  assert(cond);
  dfk_t* dfk = cond->dfk;
  DFK_DBG(dfk, "{%p} fibers waiting: %llu",
      (void*) cond, (unsigned long long) dfk_list_size(&cond->_waitqueue));
  if (!dfk_list_empty(&cond->_waitqueue)) {
    dfk_fiber_t* fiber = TO_FIBER(dfk_list_front(&cond->_waitqueue));
    DFK_DBG(dfk, "{%p} wake up {%p}", (void*) cond, (void*) fiber);
    dfk_list_pop_front(&cond->_waitqueue);
    dfk__resume(dfk->_scheduler, fiber);
  }
}

void dfk_cond_broadcast(dfk_cond_t* cond)
{
  assert(cond);
  dfk_t* dfk = cond->dfk;
  DFK_DBG(dfk, "{%p} fibers waiting: %llu",
      (void*) cond, (unsigned long long) dfk_list_size(&cond->_waitqueue));
  dfk_list_t waitqueue;
  dfk_list_init(&waitqueue);
  dfk_list_move(&cond->_waitqueue, &waitqueue);
  while (!dfk_list_empty(&waitqueue)) {
    dfk_fiber_t* fiber = TO_FIBER(dfk_list_front(&waitqueue));
    dfk_list_pop_front(&waitqueue);
    DFK_DBG(dfk, "{%p} wake up {%p}", (void*) cond, (void*) fiber);
    dfk__resume(dfk->_scheduler, fiber);
  }
}

