/**
 * @copyright
 * Copyright (c) 2015-2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <dfk.h>
#include <dfk/internal.h>

#if DFK_STACK_GUARD_SIZE
#if DFK_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#if DFK_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#endif

#if DFK_IGNORE_SIGPIPE
#include <signal.h>
#endif

#if DFK_THREADS
#include <pthread.h>
#endif

#if DFK_VALGRIND
#include <valgrind/valgrind.h>
#endif

struct coro_context init;

#if DFK_DEBUG
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
#if DFK_THREADS
  printf("[%.5s] %u %s\n", strchannel, (unsigned int) pthread_self(), msg);
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
  dfk_list_init(&dfk->_pending_coros);
  dfk_list_init(&dfk->_iowait_coros);
  dfk_list_init(&dfk->_terminated_coros);
  dfk_list_init(&dfk->_http_servers);
  dfk->_current = NULL;
  dfk->_scheduler = NULL;
  dfk->_eventloop = NULL;
  dfk->_uvloop = NULL;
  pthread_mutex_init(&dfk->_uvloop_m, NULL);
  dfk->_stopped = 0;
  dfk->malloc = dfk__default_malloc;
  dfk->free = dfk__default_free;
  dfk->realloc = dfk__default_realloc;
#if DFK_DEBUG
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
  dfk_list_free(&dfk->_terminated_coros);
  dfk_list_free(&dfk->_iowait_coros);
  dfk_list_free(&dfk->_pending_coros);
  pthread_mutex_destroy(&dfk->_uvloop_m);
  return dfk_err_ok;
}


static void dfk__terminator_cb(uv_handle_t* h, void* arg)
{
  dfk_t* dfk = (dfk_t*) arg;
  DFK_DBG(dfk, "{%p} close handle {%p}", (void*) dfk, (void*) h);
  if (!uv_is_closing(h)) {
    uv_close(h, NULL);
  }
  dfk->_stopped = 4;
}


static void dfk__terminator(dfk_coro_t* coro, void* p)
{
  dfk_t* dfk = coro->dfk;
  DFK_DBG(dfk, "hasta la vista, {%p}", (void*) dfk);
  DFK_UNUSED(p);
  DFK_UNUSED(coro);
  {
    dfk_list_hook_t* i = dfk->_http_servers.head;
    DFK_DBG(dfk, "{%p} stop http servers", (void*) dfk);
    while (i) {
      dfk_http_stop((dfk_http_t*) i);
      /* dfk_http_stop should remove self from _http_servers list */
      assert(i != dfk->_http_servers.head);
      i = dfk->_http_servers.head;
    }
  }
  DFK_DBG(dfk, "{%p} close other event handles", (void*) dfk);
  uv_walk(dfk->_uvloop, dfk__terminator_cb, dfk);
}


int dfk_stop(dfk_t* dfk)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) dfk);
  pthread_mutex_lock(&dfk->_uvloop_m);
  if (dfk->_uvloop && !dfk->_stopped) {
    /* Don't forget to unlock mutex */
    dfk->_stopped = 2;
    DFK_SYSCALL(dfk, uv_async_send(&dfk->_stop));
    DFK_DBG(dfk, "uv_async signal {%p} submitted", (void*) &dfk->_stop);
  }
  pthread_mutex_unlock(&dfk->_uvloop_m);
  return dfk_err_ok;
}


typedef struct {
  void (*ep)(dfk_t*, void*);
  void* arg;
} dfk_coro_main_arg_t;


static void dfk__coro_main(void* arg)
{
  dfk_coro_t* coro = (dfk_coro_t*) arg;
  coro->_ep(coro, coro->_arg);
  dfk_list_append(&coro->dfk->_terminated_coros, &coro->_hook);
  dfk_yield(coro, coro->dfk->_scheduler);
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
    if (!coro) {
      dfk->dfk_errno = dfk_err_nomem;
      return NULL;
    }
    char* stack_base = (char*) coro + sizeof(dfk_coro_t);
    char* stack_end = (char*) coro + dfk->default_stack_size;
    if (argsize) {
      memcpy(stack_base, arg, argsize);
      coro->_arg = stack_base;
      stack_base += argsize;
    } else {
      coro->_arg = arg;
    }

#if DFK_STACK_GUARD_SIZE
    mprotect(stack_base, DFK_STACK_GUARD_SIZE, PROT_NONE);
    stack_base += DFK_STACK_GUARD_SIZE;
#endif

    /* Align stack_base */
    if ((ptrdiff_t) stack_base % DFK_STACK_ALIGNMENT) {
      size_t padding = DFK_STACK_ALIGNMENT - ((ptrdiff_t) stack_base % DFK_STACK_ALIGNMENT);
      DFK_DBG(dfk, "stack pointer %p is not align to %d byte border, adjust by %lu bytes",
          (void*) stack_base, DFK_STACK_ALIGNMENT, (unsigned long) padding);
      stack_base += padding;
    }

#if DFK_VALGRIND
    coro->_stack_id = VALGRIND_STACK_REGISTER(stack_base, stack_end);
#endif
    coro->dfk = dfk;
    dfk_list_hook_init(&coro->_hook);
    coro->_ep = ep;
#if DFK_NAMED_COROUTINES
    snprintf(coro->_name, sizeof(coro->_name), "%p", (void*) coro);
#endif
    assert(stack_end > stack_base);
    size_t stack_size = stack_end - stack_base;
    DFK_INFO(dfk, "stack %p (%lu bytes) = {%p}",
        (void*) stack_base, (unsigned long) stack_size, (void*) coro);
    dfk_list_append(&dfk->_pending_coros, &coro->_hook);
    coro_create(&coro->_ctx, dfk__coro_main, coro, stack_base, stack_size);
    return coro;
  }
}


int dfk_coro_name(dfk_coro_t* coro, const char* fmt, ...)
{
  if (!coro || !fmt) {
    return dfk_err_badarg;
  }
  {
#if DFK_NAMED_COROUTINES
    va_list args;
    va_start(args, fmt);
    snprintf(coro->_name, sizeof(coro->_name), fmt, args);
    DFK_DBG(coro->dfk, "{%p} is now known as %s", (void*) coro, coro->_name);
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
#if DFK_VALGRIND
  VALGRIND_STACK_DEREGISTER(coro->_stack_id);
#endif
  DFK_FREE(coro->dfk, coro);
}


static void dfk_scheduler(dfk_coro_t* scheduler, void* p)
{
  dfk_t* dfk;
  DFK_UNUSED(p);
  assert(scheduler);
  dfk = scheduler->dfk;
  assert(dfk);
  DFK_DBG(dfk, "{%p} initialize event loop", (void*) dfk);
  dfk->_current = dfk->_eventloop;
  dfk_yield(scheduler, dfk->_eventloop);
  dfk->_current = scheduler;
  while (1) {
    DFK_DBG(dfk, "coroutines pending: %lu, terminated: %lu, iowait: %lu",
        (unsigned long) dfk_list_size(&dfk->_pending_coros),
        (unsigned long) dfk_list_size(&dfk->_terminated_coros),
        (unsigned long) dfk_list_size(&dfk->_iowait_coros));

    if (!dfk_list_size(&dfk->_terminated_coros)
        && !dfk_list_size(&dfk->_pending_coros)
        && !dfk_list_size(&dfk->_iowait_coros)) {
      DFK_DBG(dfk, "{%p} no pending coroutines, maybe cleanup event loop {%p}",
              (void*) dfk, (void*) dfk->_eventloop);
      pthread_mutex_lock(&dfk->_uvloop_m);
      dfk->_stopped = 3;
      int need_cleanup = !!dfk->_uvloop;
      pthread_mutex_unlock(&dfk->_uvloop_m);
      DFK_DBG(dfk, "{%p} %sneed to cleanup event loop {%p}",
              (void*) dfk, need_cleanup ? "" : "no ", (void*) dfk->_eventloop);
      if (need_cleanup) {
        dfk->_current = dfk->_eventloop;
        dfk_yield(scheduler, dfk->_eventloop);
        dfk->_current = scheduler;
      } else {
        dfk->_eventloop = NULL;
      }
      break;
    }

    {
      DFK_DBG(dfk, "{%p} cleanup terminated coroutines", (void*) dfk);
      dfk_coro_t* i = (dfk_coro_t*) dfk->_terminated_coros.head;
      dfk_list_clear(&dfk->_terminated_coros);
      while (i) {
        dfk_coro_t* next = (dfk_coro_t*) i->_hook.next;
        DFK_DBG(dfk, "corotine {%p} is terminated, cleanup", (void*) i);
        dfk__coro_free(i);
        i = next;
      }
    }

    {
      DFK_DBG(dfk, "{%p} execute pending coroutines", (void*) dfk);
      dfk_coro_t* i = (dfk_coro_t*) dfk->_pending_coros.head;
      dfk_list_clear(&dfk->_pending_coros);
      while (i) {
        dfk_coro_t* next = (dfk_coro_t*) i->_hook.next;
        DFK_DBG(dfk, "next coroutine to run {%p}", (void*) i);
        dfk->_current = i;
        dfk_yield(scheduler, i);
        i = next;
      }
    }

    if (!dfk_list_size(&dfk->_pending_coros)
        && dfk_list_size(&dfk->_iowait_coros))
    {
      /* pending_coros list is empty - switch to IO with possible blocking */
      DFK_DBG(dfk, "no pending coroutines, will do I/O");
      dfk_yield(scheduler, dfk->_eventloop);
    }
  }
  DFK_INFO(dfk, "no pending coroutines left in execution queue, jobs done");
  dfk->_current = NULL;
  dfk_yield(scheduler, NULL);
}


static void dfk_eventloop(dfk_coro_t* coro, void* p)
{
  DFK_UNUSED(p);
  assert(coro);
  dfk_t* dfk = coro->dfk;
  assert(dfk);
  uv_loop_t loop;
  uv_loop_init(&loop);
  pthread_mutex_lock(&dfk->_uvloop_m);
  dfk->_uvloop = &loop;
  DFK_SYSCALL_RVOID(dfk, uv_async_init(dfk->_uvloop, &dfk->_stop, NULL));
  pthread_mutex_unlock(&dfk->_uvloop_m);
  DFK_DBG(dfk, "initialized");
  dfk_yield(coro, dfk->_scheduler);
  pthread_mutex_lock(&dfk->_uvloop_m);
  while (uv_loop_alive(&loop)) {
    switch (dfk->_stopped) {
      case 0:
      case 1:
        DFK_DBG(dfk, "{%p} poll", (void*) &loop);
        pthread_mutex_unlock(&dfk->_uvloop_m);
        if (uv_run(&loop, UV_RUN_ONCE) == 0) {
          DFK_DBG(dfk, "{%p} no more active handlers", (void*) &loop);
        }
        dfk_yield(coro, dfk->_scheduler);
        pthread_mutex_lock(&dfk->_uvloop_m);
        break;
      case 2: /* dfk_stop was called */
        dfk->_stopped = 1;
        DFK_DBG(dfk, "{%p} stopped flag is set, lauch terminator", (void*) dfk);
        dfk_coro_t* c = dfk_run(dfk, dfk__terminator, NULL, 0);
        if (!c) {
          DFK_ERROR(dfk, "{%p} dfk_run returned %d (%s)", (void*) dfk,
              dfk->dfk_errno, dfk_strerr(dfk, dfk->dfk_errno));
        } else {
          DFK_CALL_RVOID(dfk, dfk_coro_name(c, "terminator"));
          dfk_yield(coro, dfk->_scheduler);
        }
        break;
      case 3:
        if (!uv_is_closing((uv_handle_t*) &dfk->_stop)) {
          uv_close((uv_handle_t*) &dfk->_stop, NULL);
        }
        uv_run(&loop, UV_RUN_DEFAULT);
        assert(!uv_loop_alive(&loop));
        break;
      case 4:
        /*
         * Run uv_close empty callbacks and stop
         */
        uv_run(&loop, UV_RUN_DEFAULT);
        assert(!uv_loop_alive(&loop));
        break;
      default:
        assert(0 && "impossible");
    }
  }

  dfk->_uvloop = NULL;
  pthread_mutex_unlock(&dfk->_uvloop_m);

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
#if DFK_NAMED_COROUTINES
  DFK_DBG((from ? from : to)->dfk, "context switch {%s} -> {%s}",
      from ? from->_name : "(nil)", to ? to->_name : "(nil)");
#else
  DFK_DBG((from ? from : to)->dfk, "context switch {%p} -> {%p}",
      (void*) from, (void*) to);
#endif
  coro_transfer(from ? &from->_ctx : &init, to ? &to->_ctx : &init);
  return dfk_err_ok;
}


int dfk_work(dfk_t* dfk)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  DFK_INFO(dfk, "start work cycle {%p}", (void*) dfk);

#if DFK_IGNORE_SIGPIPE
  (void) signal(SIGPIPE, SIG_IGN);
#endif

  dfk->_scheduler = dfk_run(dfk, dfk_scheduler, NULL, 0);
  if (!dfk->_scheduler) {
    return dfk->dfk_errno;
  }
  DFK_CALL(dfk, dfk_coro_name(dfk->_scheduler, "scheduler"));
  /* Exclude scheduler from run queue */
  assert((dfk_coro_t*) dfk->_pending_coros.tail == dfk->_scheduler);
  dfk_list_pop_back(&dfk->_pending_coros);

  dfk->_eventloop = dfk_run(dfk, dfk_eventloop, NULL, 0);
  if (!dfk->_eventloop) {
    dfk__coro_free(dfk->_scheduler);
    return dfk->dfk_errno;
  }
  DFK_CALL(dfk, dfk_coro_name(dfk->_eventloop, "eventloop"));
  /* Exclude event_loop from run queue */
  assert((dfk_coro_t*) dfk->_pending_coros.tail == dfk->_eventloop);
  dfk_list_pop_back(&dfk->_pending_coros);

  DFK_CALL(dfk, dfk_yield(NULL, dfk->_scheduler));

  /* Scheduler and eventloop are still in "terminated" state
   * Cleanup them manually, if required
   */
  dfk_list_clear(&dfk->_terminated_coros);
  dfk__coro_free(dfk->_scheduler);
  dfk->_scheduler = NULL;
  if (dfk->_eventloop) {
    dfk__coro_free(dfk->_eventloop);
    dfk->_eventloop = NULL;
  }
  DFK_INFO(dfk, "work cycle {%p} done", (void*) dfk);
  return dfk_err_ok;
}


static void dfk__sleep_callback(uv_timer_t* timer)
{
  assert(timer);
  dfk_coro_t* coro = (dfk_coro_t*) timer->data;
  DFK_DBG(coro->dfk, "{%p} wake up", (void*) coro);
  DFK_IO_RESUME(coro->dfk, coro);
}


int dfk_sleep(dfk_t* dfk, uint64_t msec)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p} sleep for %llu millisecond", (void*) dfk, (unsigned long long) msec);
  if (!msec) {
    DFK_DBG(dfk, "{%p} sleep for 0 seconds, postpone current coroutine %p",
        (void*) dfk, (void*) DFK_THIS_CORO(dfk));
    DFK_POSTPONE(dfk);
    return dfk_err_ok;
  }
  uv_timer_t timer;
  uv_timer_init(dfk->_uvloop, &timer);
  timer.data = DFK_THIS_CORO(dfk);
  uv_timer_start(&timer, dfk__sleep_callback, msec, 0);
  DFK_IO(dfk);
  uv_timer_stop(&timer);
  uv_close((uv_handle_t*) &timer, NULL);
  return dfk_err_ok;
}


const char* dfk_strerr(dfk_t* dfk, int err)
{
  switch(err) {
    case dfk_err_ok: {
      return "dfk_err_ok(0): No error";
    }
    case dfk_err_eof: {
      return "dfk_err_eof(1): End of file (stream, iterator)";
    }
    case dfk_err_busy: {
      return "dfk_err_busy(2): Resource is already acquired";
    }
    case dfk_err_nomem: {
      return "dfk_err_nomem(3): Memory allocation function returned NULL";
    }
    case dfk_err_notfound: {
      return "dfk_err_notfound(4): Object not found";
    }
    case dfk_err_badarg: {
      return "dfk_err_badarg(5): Bad argument";
    }
    case dfk_err_sys: {
      return dfk ? strerror(dfk->sys_errno) :
        "dfk_err_sys(6): System error, dfk_t object is NULL, "
        "can not access sys_errno";
    }
    case dfk_err_inprog: {
      return "dfk_err_inprog(7): The operation is already in progress";
    }
    case dfk_err_panic: {
      return "dfk_err_panic(8): Unexpected behaviour";
    }
    case dfk_err_not_implemented: {
      return "dfk_err_not_implemented(9): Functionality is not implemented yet";
    }
    case dfk_err_overflow: {
      return "dfk_err_overflow(10): Floating point, or integer overflow error";
    }
    case dfk_err_protocol: {
      return "dfk_err_protocol(11): Protocol violation";
    }
    default: {
      return "Unknown error";
    }
  }
  return "Unknown error";
}

