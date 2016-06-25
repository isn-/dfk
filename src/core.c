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

#ifdef DFK_IGNORE_SIGPIPE
#include <signal.h>
#endif

#ifdef DFK_THREADS
#include <pthread.h>
#endif

#ifdef DFK_VALGRIND
#include <valgrind/valgrind.h>
#endif

struct coro_context init;

#ifdef DFK_DEBUG
static void dfk__default_log(dfk_t* dfk, int channel, const char* msg)
{
  char strchannel[5] = {0};
  DFK_UNUSED(dfk);
  switch(channel) {
    case dfk_log_error: memcpy(strchannel, "error", 5); break;
    case dfk_log_warning: memcpy(strchannel, "warn_", 5); break;
    case dfk_log_info: memcpy(strchannel, "info_", 5); break;
    case dfk_log_debug: memcpy(strchannel, "debug", 5); break;
    default: snprintf(strchannel, sizeof(strchannel), "%5d", channel);
  }
#ifdef DFK_THREADS
  printf("[%.5s] %d %s\n", strchannel, (int) pthread_self(), msg);
#else
  printf("[%.5s] %s\n", strchannel, msg);
#endif
  fflush(stdout);
}
#endif /* DFK_DEBUG */


static void* dfk__default_malloc(dfk_t* dfk, size_t size)
{
  void* res;
  res = malloc(size);
  DFK_DBG(dfk, "%lu bytes requested = %p",
      (unsigned long) size, res);
  return res;
}


static void dfk__default_free(dfk_t* dfk, void* p)
{
  DFK_DBG(dfk, "release memory %p", (void*) p);
  free(p);
}


static void* dfk__default_realloc(dfk_t* dfk, void* p, size_t size)
{
  void* res;
  res = realloc(p, size);
  DFK_DBG(dfk, "resize %p to %lu bytes requested = %p",
      p, (unsigned long) size, res);
  return res;
}


int dfk_init(dfk_t* dfk)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  dfk_list_init(&dfk->_.pending_coros);
  dfk_list_init(&dfk->_.iowait_coros);
  dfk_list_init(&dfk->_.terminated_coros);
  dfk->_.current = NULL;
  dfk->_.scheduler = NULL;
  dfk->_.eventloop = NULL;
  dfk->malloc = dfk__default_malloc;
  dfk->free = dfk__default_free;
  dfk->realloc = dfk__default_realloc;
#ifdef DFK_DEBUG
  dfk->log = dfk__default_log;
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
  dfk_list_free(&dfk->_.iowait_coros);
  dfk_list_free(&dfk->_.pending_coros);
  return dfk_err_ok;
}


static void dfk__terminator_cb(uv_handle_t* h, void* arg)
{
  dfk_t* dfk = (dfk_t*) arg;
  DFK_DBG(dfk, "{%p} close handle {%p}", (void*) dfk, (void*) h);
  uv_close(h, NULL);
}


static void dfk__terminator(dfk_coro_t* coro, void* p)
{
  dfk_t* dfk = coro->dfk;
  DFK_UNUSED(p);
  DFK_UNUSED(coro);
  {
    dfk_list_hook_t* i = dfk->_.http_servers.head;
    DFK_DBG(dfk, "{%p} stop http servers", (void*) dfk);
    dfk_list_clear(&dfk->_.http_servers);
    while (i) {
      dfk_http_stop((dfk_http_t*) i);
      i = i->next;
    }
  }
  DFK_DBG(dfk, "{%p} close other event handles", (void*) dfk);
  uv_walk(dfk->_.uvloop, dfk__terminator_cb, dfk);
}


static void dfk__stop(uv_async_t* h)
{
  dfk_t* dfk = (dfk_t*) h->data;
  DFK_DBG(dfk, "{%p}", (void*) dfk);
  uv_close((uv_handle_t*) h, NULL);
  {
    dfk_coro_t* c = dfk_run(dfk, dfk__terminator, NULL, 0);
    if (!c) {
      DFK_ERROR(dfk, "{%p} dfk_run returned %d (%s)", (void*) dfk,
          dfk->dfk_errno, dfk_strerr(dfk, dfk->dfk_errno));
    }
    DFK_CALL_RVOID(dfk, dfk_coro_name(c, "terminator"));
  }
}


int dfk_stop(dfk_t* dfk)
{
  if (!dfk || !dfk->_.uvloop) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) dfk);
  DFK_SYSCALL(dfk, uv_async_init(dfk->_.uvloop, &dfk->_.stop, dfk__stop));
  dfk->_.stop.data = dfk;
  DFK_SYSCALL(dfk, uv_async_send(&dfk->_.stop));
  return dfk_err_ok;
}


typedef struct {
  void (*ep)(dfk_t*, void*);
  void* arg;
} dfk_coro_main_arg_t;


static void dfk__coro_main(void* arg)
{
  dfk_coro_t* coro = (dfk_coro_t*) arg;
  coro->_.ep(coro, coro->_.arg);
  dfk_list_append(&coro->dfk->_.terminated_coros, &coro->_.hook);
  dfk_yield(coro, coro->dfk->_.scheduler);
}


dfk_coro_t* dfk_run(dfk_t* dfk, void (*ep)(dfk_coro_t*, void*), void* arg, size_t argsize)
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
    if (argsize) {
      memcpy(stack_base, arg, argsize);
      stack_base += argsize;
      stack_size -= argsize;
    }
#ifdef DFK_VALGRIND
    coro->_.stack_id = VALGRIND_STACK_REGISTER(stack_base, stack_base + stack_size);
#endif
    coro->dfk = dfk;
    dfk_list_hook_init(&coro->_.hook);
    coro->_.ep = ep;
    if (argsize) {
      coro->_.arg = stack_base - argsize;
    } else {
      coro->_.arg = arg;
    }
#ifdef DFK_NAMED_COROUTINES
    snprintf(coro->_.name, sizeof(coro->_.name), "%p", (void*) coro);
#endif
    DFK_INFO(dfk, "stack %p (%lu bytes) = {%p}",
        (void*) stack_base, (unsigned long) stack_size, (void*) coro);
    dfk_list_append(&dfk->_.pending_coros, &coro->_.hook);
    coro_create(&coro->_.ctx, dfk__coro_main, coro, stack_base, stack_size);
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


static void dfk__coro_free(dfk_coro_t* coro)
{
  assert(coro);
#ifdef DFK_VALGRIND
  VALGRIND_STACK_DEREGISTER(coro->_.stack_id);
#endif
  DFK_FREE(coro->dfk, coro);
}


static void dfk__scheduler(dfk_coro_t* scheduler, void* p)
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
    DFK_DBG(dfk, "coroutines pending: %lu, terminated: %lu, iowait: %lu",
        (unsigned long) dfk_list_size(&dfk->_.pending_coros),
        (unsigned long) dfk_list_size(&dfk->_.terminated_coros),
        (unsigned long) dfk_list_size(&dfk->_.iowait_coros));

    if (!dfk_list_size(&dfk->_.terminated_coros)
        && !dfk_list_size(&dfk->_.pending_coros)
        && !dfk_list_size(&dfk->_.iowait_coros)) {
      /* Cleanup event loop*/
      dfk->_.current = dfk->_.eventloop;
      dfk_yield(scheduler, dfk->_.eventloop);
      dfk->_.current = scheduler;
      break;
    }

    {
      /* Cleanup terminated coroutines */
      dfk_coro_t* i = (dfk_coro_t*) dfk->_.terminated_coros.head;
      dfk_list_clear(&dfk->_.terminated_coros);
      while (i) {
        dfk_coro_t* next = (dfk_coro_t*) i->_.hook.next;
        DFK_DBG(dfk, "corotine {%p} is terminated, cleanup", (void*) i);
        dfk__coro_free(i);
        i = next;
      }
    }

    {
      /* Execute pending coroutines */
      dfk_coro_t* i = (dfk_coro_t*) dfk->_.pending_coros.head;
      dfk_list_clear(&dfk->_.pending_coros);
      while (i) {
        dfk_coro_t* next = (dfk_coro_t*) i->_.hook.next;
        DFK_DBG(dfk, "next coroutine to run {%p}", (void*) i);
        dfk->_.current = i;
        dfk_yield(scheduler, i);
        i = next;
      }
    }

    if (!dfk_list_size(&dfk->_.pending_coros)
        && dfk_list_size(&dfk->_.iowait_coros))
    {
      /* pending_coros list is empty - switch to IO with possible blocking */
      DFK_DBG(dfk, "no pending coroutines, will do I/O");
      dfk_yield(scheduler, dfk->_.eventloop);
    }
  }
  DFK_INFO(dfk, "no pending coroutines left in execution queue, jobs done");
  dfk->_.current = NULL;
  dfk_yield(scheduler, NULL);
}


static void dfk__eventloop(dfk_coro_t* coro, void* p)
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
    if (uv_run(&loop, UV_RUN_ONCE) == 0) {
      DFK_DBG(dfk, "{%p} no more active handlers", (void*) &loop);
    }
    dfk_yield(coro, dfk->_.scheduler);
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

#ifdef DFK_IGNORE_SIGPIPE
  (void) signal(SIGPIPE, SIG_IGN);
#endif

  dfk->_.scheduler = dfk_run(dfk, dfk__scheduler, NULL, 0);
  if (!dfk->_.scheduler) {
    return dfk->dfk_errno;
  }
  DFK_CALL(dfk, dfk_coro_name(dfk->_.scheduler, "scheduler"));
  /* Exclude scheduler from run queue */
  assert((dfk_coro_t*) dfk->_.pending_coros.tail == dfk->_.scheduler);
  dfk_list_pop_back(&dfk->_.pending_coros);

  dfk->_.eventloop = dfk_run(dfk, dfk__eventloop, NULL, 0);
  if (!dfk->_.eventloop) {
    dfk__coro_free(dfk->_.scheduler);
    return dfk->dfk_errno;
  }
  DFK_CALL(dfk, dfk_coro_name(dfk->_.eventloop, "eventloop"));
  /* Exclude event_loop from run queue */
  assert((dfk_coro_t*) dfk->_.pending_coros.tail == dfk->_.eventloop);
  dfk_list_pop_back(&dfk->_.pending_coros);

  DFK_CALL(dfk, dfk_yield(NULL, dfk->_.scheduler));

  /* Scheduler and eventloop are still in "terminated" state
   * Cleanup them manually
   */
  dfk_list_clear(&dfk->_.terminated_coros);
  dfk__coro_free(dfk->_.scheduler);
  dfk__coro_free(dfk->_.eventloop);
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
    case dfk_err_eof: {
      return "End of file (stream, iterator)";
    }
    case dfk_err_busy: {
      return "Resource is already acquired";
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

