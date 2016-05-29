/**
 * @copyright
 * Copyright (c) 2015, 2016, Stanislav Ivochkin. All Rights Reserved.
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <dfk.h>
#include <dfk/internal.h>

#ifdef DFK_VALGRIND
#include <valgrind/valgrind.h>
#endif

struct coro_context init;

#ifdef DFK_DEBUG
static void dfk_default_log(void* ud, int channel, const char* msg)
{
  char strchannel[5] = {0};
  DFK_UNUSED(ud);
  switch(channel) {
    case dfk_log_error: memcpy(strchannel, "error", 5); break;
    case dfk_log_warning: memcpy(strchannel, "warn_", 5); break;
    case dfk_log_info: memcpy(strchannel, "info_", 5); break;
    case dfk_log_debug: memcpy(strchannel, "debug", 5); break;
    default: snprintf(strchannel, sizeof(strchannel), "%5d", channel);
  }
  /* At most 512 bytes will printed at once */
  printf("[%.5s] %.503s\n", strchannel, msg);
  fflush(stdout);
}
#endif /* DFK_DEBUG */


static void* dfk_default_malloc(void* dfk, size_t size)
{
  void* err;
  err = malloc(size);
  DFK_DBG((dfk_t*) dfk, "%lu bytes requested = %p",
      (unsigned long) size, err);
  return err;
}


static void dfk_default_free(void* dfk, void* p)
{
  DFK_DBG((dfk_t*) dfk, "release memory %p", (void*) p);
  free(p);
}


static void* dfk_default_realloc(void* dfk, void* p, size_t size)
{
  void* err;
  err = realloc(p, size);
  DFK_DBG((dfk_t*) dfk, "resize %p to %lu bytes requested = %p",
      p, (unsigned long) size, err);
  return err;
}


int dfk_init(dfk_t* dfk)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  dfk_list_init(&dfk->_.pending_coros);
  dfk_list_init(&dfk->_.terminated_coros);
  dfk->_.current = NULL;
  dfk->_.scheduler = NULL;
  dfk->_.eventloop = NULL;
  dfk->malloc = dfk_default_malloc;
  dfk->free = dfk_default_free;
  dfk->realloc = dfk_default_realloc;
#ifdef DFK_DEBUG
  dfk->log = dfk_default_log;
#else
  dfk->log = NULL;
#endif
  dfk->default_stack_size = DFK_STACK_SIZE;

  dfk->sys_errno = 0;
  dfk->dfk_errno = 0;

  return dfk_err_ok;
}


int dfk_free(dfk_t* dfk)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  dfk_list_free(&dfk->_.terminated_coros);
  dfk_list_free(&dfk->_.pending_coros);
  return dfk_err_ok;
}


typedef struct {
  void (*ep)(dfk_t*, void*);
  void* arg;
} dfk_coro_main_arg_t;


static void dfk_coro_main(void* arg)
{
  dfk_coro_t* coro = (dfk_coro_t*) arg;
  coro->_.ep(coro, coro->_.arg);
  dfk_list_append(&coro->dfk->_.terminated_coros, &coro->_.hook);
  dfk_yield(coro, coro->dfk->_.scheduler);
}


dfk_coro_t* dfk_run(dfk_t* dfk, void (*ep)(dfk_coro_t*, void*), void* arg)
{
  if (!dfk) {
    return NULL;
  }
  if (!ep) {
    dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  {
    dfk_coro_t* coro = DFK_MALLOC(dfk, dfk->default_stack_size);
    char* stack_base = (char*) coro + sizeof(dfk_coro_t);
    size_t stack_size = dfk->default_stack_size - sizeof(dfk_coro_t);
    if (!coro) {
      dfk->dfk_errno = dfk_err_nomem;
      return NULL;
    }
#ifdef DFK_VALGRIND
    coro->_.stack_id = VALGRIND_STACK_REGISTER(stack_base, stack_base + stack_size);
#endif
    coro->dfk = dfk;
    dfk_list_hook_init(&coro->_.hook);
    coro->_.ep = ep;
    coro->_.arg = arg;
#ifdef DFK_NAMED_COROUTINES
    snprintf(coro->_.name, sizeof(coro->_.name), "%p", (void*) coro);
#endif
    DFK_INFO(dfk, "stack %p (%lu bytes) = {%p}",
        (void*) stack_base, (unsigned long) stack_size, (void*) coro);
    dfk_list_append(&dfk->_.pending_coros, &coro->_.hook);
    coro_create(&coro->_.ctx, dfk_coro_main, coro, stack_base, stack_size);
    return coro;
  }
}


int dfk_coro_name(dfk_coro_t* coro, const char* fmt, ...)
{
  if (!coro || !fmt) {
    return dfk_err_badarg;
  }
  {
#ifdef DFK_NAMED_COROUTINES
    va_list args;
    va_start(args, fmt);
    snprintf(coro->_.name, sizeof(coro->_.name), fmt, args);
    DFK_DBG(coro->dfk, "{%p} is now known as %s", (void*) coro, coro->_.name);
    va_end(args);
#else
    DFK_UNUSED(coro);
    DFK_UNUSED(fmt);
#endif
    return dfk_err_ok;
  }
}


static int dfk_coro_free(dfk_coro_t* coro)
{
  if (!coro) {
    return dfk_err_badarg;
  }
#ifdef DFK_VALGRIND
  VALGRIND_STACK_DEREGISTER(coro->_.stack_id);
#endif
  DFK_FREE(coro->dfk, coro);
  return dfk_err_ok;
}


static void dfk_scheduler(dfk_coro_t* scheduler, void* p)
{
  dfk_t* dfk;
  DFK_UNUSED(p);
  assert(scheduler);
  dfk = scheduler->dfk;
  assert(dfk);
  /* Initialize event loop */
  dfk->_.current = dfk->_.eventloop;
  dfk_yield(scheduler, dfk->_.eventloop);
  dfk->_.current = scheduler;
  while (1) {
    if (!dfk_list_size(&dfk->_.terminated_coros)
        && !dfk_list_size(&dfk->_.pending_coros)) {
      /* Cleanup event loop*/
      dfk->_.current = dfk->_.eventloop;
      dfk_yield(scheduler, dfk->_.eventloop);
      dfk->_.current = scheduler;
      break;
    }
    DFK_DBG(dfk, "coroutines pending: %lu, terminated: %lu",
        (unsigned long) dfk_list_size(&dfk->_.pending_coros),
        (unsigned long) dfk_list_size(&dfk->_.terminated_coros));
    {
      /* Cleanup terminated coroutines */
      dfk_coro_t* i = (dfk_coro_t*) dfk->_.terminated_coros.head;
      dfk_list_clear(&dfk->_.terminated_coros);
      while (i) {
        dfk_coro_t* n = (dfk_coro_t*) i->_.hook.next;
        DFK_DBG(dfk, "corotine {%p} is terminated, cleanup", (void*) i);
        dfk_coro_free(i);
        i = n;
      }
    }
    /* Execute coroutine from the front on pending_coros list */
    if (dfk_list_size(&dfk->_.pending_coros)) {
      dfk->_.current = (dfk_coro_t*) dfk->_.pending_coros.head;
      dfk_list_pop_front(&dfk->_.pending_coros);
      DFK_DBG(dfk, "next coroutine to run {%p}", (void*) dfk->_.current);
      dfk_yield(scheduler, dfk->_.current);
      dfk->_.current = scheduler;
    }
  }
  DFK_INFO(dfk, "no pending coroutines left in execution queue, jobs done");
  dfk->_.current = NULL;
  dfk_yield(scheduler, NULL);
}


static void dfk_event_loop(dfk_coro_t* coro, void* p)
{
  uv_loop_t loop;
  dfk_t* dfk;
  DFK_UNUSED(p);
  assert(coro);
  dfk = coro->dfk;
  assert(dfk);
  uv_loop_init(&loop);
  dfk->_.uvloop = &loop;
  DFK_DBG(dfk, "initialized");
  dfk_yield(coro, dfk->_.scheduler);
  while (uv_loop_alive(&loop)) {
    DFK_DBG(dfk, "{%p} poll", (void*) &loop);
    if (uv_run(&loop, UV_RUN_DEFAULT) == 0) {
      DFK_DBG(dfk, "{%p} no more active handlers", (void*) &loop);
    }
  }
  {
    int err = uv_loop_close(&loop);
    DFK_DBG(dfk, "{%p} uv_loop_close() returned %d", (void*) &loop, err);
    DFK_UNUSED(err);
  }
  DFK_DBG(dfk, "terminated");
}


int dfk_yield(dfk_coro_t* from, dfk_coro_t* to)
{
  if (!from && !to) {
    return dfk_err_badarg;
  }
#ifdef DFK_NAMED_COROUTINES
  DFK_DBG((from ? from : to)->dfk, "context switch {%s} -> {%s}",
      from ? from->_.name : "(nil)", to ? to->_.name : "(nil)");
#else
  DFK_DBG((from ? from : to)->dfk, "context switch {%p} -> {%p}",
      (void*) from, (void*) to);
#endif
  coro_transfer(from ? &from->_.ctx : &init, to ? &to->_.ctx : &init);
  return dfk_err_ok;
}


int dfk_work(dfk_t* dfk)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  DFK_INFO(dfk, "start work cycle {%p}", (void*) dfk);

  dfk->_.scheduler = dfk_run(dfk, dfk_scheduler, NULL);
  if (!dfk->_.scheduler) {
    return dfk->dfk_errno;
  }
  DFK_CALL(dfk, dfk_coro_name(dfk->_.scheduler, "scheduler"));
  /* Exclude scheduler from run queue */
  assert((dfk_coro_t*) dfk->_.pending_coros.tail == dfk->_.scheduler);
  dfk_list_pop_back(&dfk->_.pending_coros);

  dfk->_.eventloop = dfk_run(dfk, dfk_event_loop, NULL);
  if (!dfk->_.eventloop) {
    return dfk->dfk_errno;
  }
  DFK_CALL(dfk, dfk_coro_name(dfk->_.eventloop, "event_loop"));
  /* Exclude event_loop from run queue */
  assert((dfk_coro_t*) dfk->_.pending_coros.tail == dfk->_.eventloop);
  dfk_list_pop_back(&dfk->_.pending_coros);

  DFK_CALL(dfk, dfk_yield(NULL, dfk->_.scheduler));

  /* Scheduler and eventloop are still in "terminated" state
   * Cleanup them manually
   */
  dfk_list_clear(&dfk->_.terminated_coros);
  DFK_CALL(dfk, dfk_coro_free(dfk->_.scheduler));
  DFK_CALL(dfk, dfk_coro_free(dfk->_.eventloop));
  dfk->_.scheduler = NULL;
  dfk->_.eventloop = NULL;
  DFK_INFO(dfk, "work cycle {%p} done", (void*) dfk);
  return dfk_err_ok;
}


const char* dfk_strerr(dfk_t* dfk, int err)
{
  switch(err) {
    case dfk_err_ok: {
      return "No error";
    }
    case dfk_err_nomem: {
      return "Memory allocation function returned NULL";
    }
    case dfk_err_notfound: {
      return "Object not found";
    }
    case dfk_err_badarg: {
      return "Bad argument";
    }
    case dfk_err_sys: {
      return dfk ? strerror(dfk->sys_errno) :
        "System error, dfk_t object is NULL, can not access sys_errno";
    }
    case dfk_err_inprog: {
      return "The operation is already in progress";
    }
    case dfk_err_panic: {
      return "Unexpected behaviour";
    }
    case dfk_err_not_implemented: {
      return "Functionality is not implemented yet";
    }
    default: {
      return "Unknown error";
    }
  }
  return "Unknown error";
}

