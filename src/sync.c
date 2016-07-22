/**
 * @copyright
 * Copyright (c) 2016, Stanislav Ivochkin. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <dfk/sync.h>
#include <dfk/internal.h>


int dfk_mutex_init(dfk_mutex_t* mutex, dfk_t* dfk)
{
  if (!mutex || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) mutex);
  dfk_list_init(&mutex->_waitqueue);
  mutex->_owner = NULL;
  mutex->dfk = dfk;
  return dfk_err_ok;
}


int dfk_mutex_free(dfk_mutex_t* mutex)
{
  if (!mutex) {
    return dfk_err_badarg;
  }
  DFK_DBG(mutex->dfk, "{%p}", (void*) mutex);
  if (mutex->_owner || dfk_list_size(&mutex->_waitqueue)) {
    DFK_ERROR(mutex->dfk, "{%p} attempt to free busy mutex", (void*) mutex);
    return dfk_err_busy;
  }
  assert(!mutex->_owner);
  assert(!dfk_list_size(&mutex->_waitqueue));
  dfk_list_free(&mutex->_waitqueue);
  return dfk_err_ok;
}


int dfk_mutex_lock(dfk_mutex_t* mutex)
{
  if (!mutex) {
    return dfk_err_badarg;
  }
  DFK_DBG(mutex->dfk, "{%p} lock attempt by {%p}",
      (void*) mutex, (void*) DFK_THIS_CORO(mutex->dfk));
  if (!mutex->_owner) {
    DFK_DBG(mutex->dfk, "{%p} is spare, acquire lock", (void*) mutex);
    mutex->_owner = DFK_THIS_CORO(mutex->dfk);
    return dfk_err_ok;
  } else {
    if (mutex->_owner == DFK_THIS_CORO(mutex->dfk)) {
      DFK_DBG(mutex->dfk, "{%p} is locked by the caller, recursive lock", (void*) mutex);
      return dfk_err_ok;
    } else {
      DFK_DBG(mutex->dfk, "{%p} is already locked by {%p}, waiting for unlock",
          (void*) mutex, (void*) mutex->_owner);
      dfk_list_append(&mutex->_waitqueue, (dfk_list_hook_t*) DFK_THIS_CORO(mutex->dfk));
      DFK_YIELD(mutex->dfk);
    }
  }
  assert(mutex->_owner == DFK_THIS_CORO(mutex->dfk));
  DFK_DBG(mutex->dfk, "{%p} is now acquired by {%p}",
      (void*) mutex, (void*) mutex->_owner);
  return dfk_err_ok;
}


int dfk_mutex_unlock(dfk_mutex_t* mutex)
{
  if (!mutex) {
    return dfk_err_badarg;
  }
  assert(mutex->_owner);
  assert(mutex->_owner == DFK_THIS_CORO(mutex->dfk));
  if (dfk_list_size(&mutex->_waitqueue)) {
    DFK_DBG(mutex->dfk, "{%p} is unlocked, resume {%p}", (void*) mutex, (void*) mutex->_waitqueue.head);
    mutex->_owner = (dfk_coro_t*) mutex->_waitqueue.head;
    dfk_list_pop_front(&mutex->_waitqueue);
    DFK_RESUME(mutex->dfk, mutex->_owner);
  } else {
    DFK_DBG(mutex->dfk, "{%p} is unlocked, no coroutine is waiting for lock", (void*) mutex);
    mutex->_owner = NULL;
  }
  return dfk_err_ok;
}


int dfk_mutex_trylock(dfk_mutex_t* mutex)
{
  if (!mutex) {
    return dfk_err_badarg;
  }
  DFK_DBG(mutex->dfk, "{%p} trylock attempt by {%p}",
      (void*) mutex, (void*) DFK_THIS_CORO(mutex->dfk));
  if (mutex->_owner) {
    if (mutex->_owner == DFK_THIS_CORO(mutex->dfk)) {
      DFK_DBG(mutex->dfk, "{%p} is locked by the caller, recursive lock", (void*) mutex);
      return dfk_err_ok;
    } else {
      DFK_DBG(mutex->dfk, "{%p} is already locked by {%p}, dfk_err_busy",
          (void*) mutex, (void*) mutex->_owner);
      return dfk_err_busy;
    }
  }
  DFK_DBG(mutex->dfk, "{%p} is spare, acquire lock", (void*) mutex);
  assert(!mutex->_owner);
  assert(!dfk_list_size(&mutex->_waitqueue));
  mutex->_owner = DFK_THIS_CORO(mutex->dfk);
  return dfk_err_ok;
}


int dfk_cond_init(dfk_cond_t* cond, dfk_t* dfk)
{
  if (!cond || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) cond);
  dfk_list_init(&cond->_waitqueue);
  cond->dfk = dfk;
  return dfk_err_ok;
}


int dfk_cond_free(dfk_cond_t* cond)
{
  if (!cond) {
    return dfk_err_badarg;
  }
  DFK_DBG(cond->dfk, "{%p}", (void*) cond);
  if (dfk_list_size(&cond->_waitqueue)) {
    DFK_ERROR(cond->dfk, "{%p} waitqueue is non-empty, dfk_err_busy", (void*) cond);
    return dfk_err_busy;
  }
  dfk_list_free(&cond->_waitqueue);
  return dfk_err_ok;
}


int dfk_cond_wait(dfk_cond_t* cond, dfk_mutex_t* mutex)
{
  if (!cond || !mutex) {
    return dfk_err_badarg;
  }
  DFK_DBG(cond->dfk, "{%p}", (void*) cond);
  assert(cond->dfk == mutex->dfk);
  assert(mutex->_owner);
  assert(mutex->_owner == DFK_THIS_CORO(cond->dfk));
  if (dfk_list_size(&mutex->_waitqueue)) {
    DFK_DBG(mutex->dfk, "{%p} is unlocked, resume {%p}", (void*) mutex, (void*) mutex->_waitqueue.head);
    mutex->_owner = (dfk_coro_t*) mutex->_waitqueue.head;
    dfk_list_pop_front(&mutex->_waitqueue);
    DFK_RESUME(mutex->dfk, mutex->_owner);
  } else {
    DFK_DBG(mutex->dfk, "{%p} is unlocked, no coroutine is waiting for lock", (void*) mutex);
    mutex->_owner = NULL;
  }
  dfk_list_append(&cond->_waitqueue, (dfk_list_hook_t*) DFK_THIS_CORO(cond->dfk));
  DFK_YIELD(cond->dfk);
  return dfk_mutex_lock(mutex);
}


int dfk_cond_signal(dfk_cond_t* cond)
{
  if (!cond) {
    return dfk_err_badarg;
  }
  DFK_DBG(cond->dfk, "{%p} coroutines waiting: %lu",
      (void*) cond, (unsigned long) dfk_list_size(&cond->_waitqueue));
  if (dfk_list_size(&cond->_waitqueue)) {
    dfk_coro_t* coro = (dfk_coro_t*) cond->_waitqueue.head;
    DFK_DBG(cond->dfk, "{%p} wake up {%p}", (void*) cond, (void*) coro);
    dfk_list_pop_front(&cond->_waitqueue);
    DFK_RESUME(cond->dfk, coro);
  }
  return dfk_err_ok;
}


int dfk_cond_broadcast(dfk_cond_t* cond)
{
  if (!cond) {
    return dfk_err_badarg;
  }
  DFK_DBG(cond->dfk, "{%p} coroutines waiting: %lu",
      (void*) cond, (unsigned long) dfk_list_size(&cond->_waitqueue));
  {
    dfk_list_hook_t* i = cond->_waitqueue.head;
    dfk_list_clear(&cond->_waitqueue);
    while (i) {
      dfk_list_hook_t* next = i->next;
      DFK_DBG(cond->dfk, "{%p} wake up {%p}", (void*) cond, (void*) i);
      DFK_RESUME(cond->dfk, (dfk_coro_t*) i);
      i = next;
    }
  }
  return dfk_err_ok;
}


int dfk_event_init(dfk_event_t* event, dfk_t* dfk)
{
  if (!event || !dfk) {
    return dfk_err_badarg;
  }
  event->_awaiting = NULL;
  event->dfk = dfk;
  return dfk_err_ok;
}


int dfk_event_free(dfk_event_t* event)
{
  if (!event) {
    return dfk_err_badarg;
  }
  if (event->_awaiting) {
    return dfk_err_busy;
  }
  return dfk_err_ok;
}


int dfk_event_wait(dfk_event_t* event)
{
  if (!event) {
    return dfk_err_badarg;
  }
  if (event->_awaiting) {
    return dfk_err_busy;
  }
  event->_awaiting = DFK_THIS_CORO(event->dfk);
  DFK_YIELD(event->dfk);
  return dfk_err_ok;
}


int dfk_event_signal(dfk_event_t* event)
{
  if (!event) {
    return dfk_err_badarg;
  }
  if (event->_awaiting) {
    DFK_RESUME(event->dfk, event->_awaiting);
    event->_awaiting = NULL;
  }
  return dfk_err_ok;
}

