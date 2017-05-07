/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <dfk/error.h>
#include <dfk/mutex.h>
#include <dfk/internal.h>
#include <dfk/scheduler.h>

#define TO_FIBER(expr) DFK_CONTAINER_OF((expr), dfk_fiber_t, _hook)

void dfk_mutex_init(dfk_mutex_t* mutex, dfk_t* dfk)
{
  assert(mutex);
  assert(dfk);
  DFK_DBG(dfk, "{%p}", (void*) mutex);
  dfk_list_init(&mutex->_waitqueue);
  mutex->_owner = NULL;
  mutex->dfk = dfk;
}

void dfk_mutex_free(dfk_mutex_t* mutex)
{
  assert(mutex);
  DFK_DBG(mutex->dfk, "{%p}", (void*) mutex);
  assert(dfk_list_empty(&mutex->_waitqueue)
      && "attempt to free busy mutex");
  assert(!mutex->_owner && "attempt to free locked mutex");
  DFK_IF_DEBUG(mutex->dfk = DFK_PDEADBEEF);
  DFK_IF_DEBUG(mutex->_owner = DFK_PDEADBEEF);
}

void dfk_mutex_lock(dfk_mutex_t* mutex)
{
  assert(mutex);

  dfk_t* dfk = mutex->dfk;
  dfk_fiber_t* this = dfk__this_fiber(dfk->_scheduler);
  DFK_DBG(dfk, "{%p} lock attempt by {%p}", (void*) mutex, (void*) this);
  if (!mutex->_owner) {
    DFK_DBG(dfk, "{%p} is spare, acquire lock", (void*) mutex);
    mutex->_owner = this;
  } else {
    if (mutex->_owner == this) {
      DFK_DBG(dfk, "{%p} is locked by the caller, recursive lock",
          (void*) mutex);
    } else {
      DFK_DBG(dfk, "{%p} is already locked by {%p}, wait queue size %llu",
          (void*) mutex, (void*) mutex->_owner,
          (unsigned long long) dfk_list_size(&mutex->_waitqueue));
      dfk_list_append(&mutex->_waitqueue, &this->_hook);
      dfk__suspend(dfk->_scheduler);
    }
  }
  assert(mutex->_owner == this);
  DFK_DBG(dfk, "{%p} is now acquired by {%p}", (void*) mutex,
      (void*) mutex->_owner);
}

void dfk_mutex_unlock(dfk_mutex_t* mutex)
{
  assert(mutex);
  dfk_t* dfk = mutex->dfk;
  /* Attempt to unlock mutex that hasn't been locked previously */
  assert(mutex->_owner);
  /* Attempt to unlock mutex locked by another fiber */
  assert(mutex->_owner == dfk__this_fiber(dfk->_scheduler));
  if (dfk_list_empty(&mutex->_waitqueue)) {
    DFK_DBG(dfk, "{%p} is now unlocked, no fiber is waiting for lock",
        (void*) mutex);
    mutex->_owner = NULL;
  } else {
    dfk_fiber_t* next = TO_FIBER(dfk_list_front(&mutex->_waitqueue));
    dfk_list_pop_front(&mutex->_waitqueue);
    DFK_DBG(dfk, "{%p} is acquired by {%p}, wait queue size %llu",
        (void*) mutex, (void*) next,
        (unsigned long long) dfk_list_size(&mutex->_waitqueue));
    mutex->_owner = next;
    dfk__resume(dfk->_scheduler, next);
  }
}

int dfk_mutex_trylock(dfk_mutex_t* mutex)
{
  assert(mutex);
  dfk_t* dfk = mutex->dfk;
  dfk_fiber_t* this = dfk__this_fiber(dfk->_scheduler);
  DFK_DBG(mutex->dfk, "{%p} trylock attempt by {%p}",
      (void*) mutex, (void*) this);
  if (mutex->_owner) {
    if (mutex->_owner == this) {
      DFK_DBG(dfk, "{%p} is locked by the caller, recursive lock",
          (void*) mutex);
      return dfk_err_ok;
    } else {
      DFK_DBG(mutex->dfk, "{%p} is already locked by {%p}, dfk_err_busy",
          (void*) mutex, (void*) TO_FIBER(dfk_list_front(&mutex->_waitqueue)));
      return dfk_err_busy;
    }
  }
  DFK_DBG(mutex->dfk, "{%p} is spare, acquire lock by {%p}", (void*) mutex,
      (void*) this);
  mutex->_owner = this;
  return dfk_err_ok;
}

