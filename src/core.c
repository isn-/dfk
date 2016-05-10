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


struct coro_context init;


static void dfk_default_log(void* ud, int channel, const char* msg)
{
  char strchannel[5] = {0};
  (void) ud;
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


static void* dfk_default_malloc(void* dfk, size_t size)
{
  void* err;
  err = malloc(size);
  DFK_DEBUG((dfk_t*) dfk, "%lu bytes requested = %p",
      (unsigned long) size, err);
  return err;
}


static void dfk_default_free(void* dfk, void* p)
{
  DFK_DEBUG((dfk_t*) dfk, "release memory %p", (void*) p);
  free(p);
}


static void* dfk_default_realloc(void* dfk, void* p, size_t size)
{
  void* err;
  err = realloc(p, size);
  DFK_DEBUG((dfk_t*) dfk, "resize %p to %lu bytes requested = %p",
      p, (unsigned long) size, err);
  return err;
}


int dfk_init(dfk_t* dfk)
{
  if (!dfk) {
    return dfk_err_badarg;
  }
  dfk->_.exechead = NULL;
  dfk->_.termhead = NULL;
  dfk->_.scheduler = NULL;
  dfk->_.eventloop = NULL;
  dfk->malloc = dfk_default_malloc;
  dfk->free = dfk_default_free;
  dfk->realloc = dfk_default_realloc;
#ifdef DFK_DEBUG_ENABLED
  dfk->log = dfk_default_log;
#else
  DFK_UNUSED(dfk_default_log);
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
  coro->_.next = coro->dfk->_.termhead;
  coro->dfk->_.termhead = coro;
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
    coro->dfk = dfk;
    coro->_.next = NULL;
    coro->_.ep = ep;
    coro->_.arg = arg;
#ifdef DFK_NAMED_COROUTINES
    snprintf(coro->_.name, sizeof(coro->_.name), "%p", (void*) coro);
#endif
    DFK_INFO(dfk, "stack %p (%lu) = {%p}", (void*) stack_base, (unsigned long) stack_size, (void*) coro);
    coro->_.next = dfk->_.exechead;
    dfk->_.exechead = coro;
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
  dfk_yield(scheduler, dfk->_.eventloop);
  while (1) {
    if (!dfk->_.termhead && !dfk->_.exechead) {
      break;
    }
    {
      /* cleanup terminated coroutines */
      dfk_coro_t* i = dfk->_.termhead;
      dfk_coro_t* n = i;
      while (i) {
        n = i->_.next;
        dfk_coro_free(i);
        i = n;
      }
      dfk->_.termhead = NULL;
    }
    if (dfk->_.exechead) {
      dfk_coro_t* coro = dfk->_.exechead;
      dfk->_.exechead = coro->_.next;
      DFK_DEBUG(dfk, "next coroutine to run {%p}", (void*) coro);
      dfk_yield(scheduler, coro);
    }
  }
  /* cleanup event loop*/
  dfk_yield(scheduler, dfk->_.eventloop);
  DFK_INFO(dfk, "no pending coroutines left in execution queue, jobs done");
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
  DFK_DEBUG(dfk, "initialized");
  dfk_yield(coro, dfk->_.scheduler);
  /* work here */
  uv_loop_close(&loop);
  DFK_DEBUG(dfk, "terminated");
}


int dfk_yield(dfk_coro_t* from, dfk_coro_t* to)
{
  if (!from && !to) {
    return dfk_err_badarg;
  }
#ifdef DFK_NAMED_COROUTINES
  DFK_DEBUG((from ? from : to)->dfk, "context switch {%s} -> {%s}",
      from ? from->_.name : "(nil)", to ? to->_.name : "(nil)");
#else
  DFK_DEBUG((from ? from : to)->dfk, "context switch {%p} -> {%p}",
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
  DFK_CALL(dfk_coro_name(dfk->_.scheduler, "scheduler"));
  /* exclude scheduler from run queue*/
  assert(dfk->_.exechead == dfk->_.scheduler);
  dfk->_.exechead = dfk->_.scheduler->_.next;

  dfk->_.eventloop = dfk_run(dfk, dfk_event_loop, NULL);
  if (!dfk->_.eventloop) {
    return dfk->dfk_errno;
  }
  DFK_CALL(dfk_coro_name(dfk->_.eventloop, "event_loop"));
  /* exclude event_loop from run queue*/
  assert(dfk->_.exechead == dfk->_.eventloop);
  dfk->_.exechead = dfk->_.eventloop->_.next;

  DFK_CALL(dfk_yield(NULL, dfk->_.scheduler));
  DFK_CALL(dfk_coro_free(dfk->_.scheduler));
  DFK_CALL(dfk_coro_free(dfk->_.eventloop));
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
    default: {
      return "Unknown error";
    }
  }
  return "Unknown error";
}

