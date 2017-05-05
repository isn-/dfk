/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dfk/config.h>
#include <dfk/internal.h>
#include <dfk/context.h>
#include <dfk/log.h>
#include <dfk/error.h>
#include <dfk/fiber.h>
#include <dfk/internal/fiber.h>
#include <dfk/scheduler.h>
#include <dfk/eventloop.h>

#if DFK_THREADS
#include <pthread.h>
#endif

size_t dfk_sizeof(void)
{
  return sizeof(dfk_t);
}

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

void dfk_init(dfk_t* dfk)
{
  assert(dfk);
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

  dfk->_current = NULL;
  dfk->_scheduler = NULL;
  dfk->_eventloop = NULL;
  dfk->_terminator = NULL;
  memset(&dfk->_comeback, 0, sizeof(dfk->_comeback));;
  dfk->_stopped = 0;
}

void dfk_free(dfk_t* dfk)
{
  DFK_UNUSED(dfk);
  assert(dfk);
}

int dfk_work(dfk_t* dfk, void (*ep)(dfk_fiber_t*, void*), void* arg,
    size_t argsize)
{
  assert(dfk);
  DFK_INFO(dfk, "start work cycle {%p}", (void*) dfk);

#if DFK_IGNORE_SIGPIPE
  (void) signal(SIGPIPE, SIG_IGN);
#endif

  dfk_fiber_t* eventloop = dfk__run(dfk, dfk__eventloop, NULL, 0);
  if (!eventloop) {
    return dfk->dfk_errno;
  }

  /* mainf stands for main fiber */
  dfk_fiber_t* mainf = dfk__run(dfk, ep, arg, argsize);
  if (!mainf) {
    /* A brutal method of cancelling fiber that has not been started yet */
    DFK_FREE(dfk, eventloop);
    return dfk->dfk_errno;
  }

  dfk_fiber_t* scheduler = dfk__run(dfk, dfk__scheduler_loop, mainf, 0);
  if (!scheduler) {
    DFK_FREE(dfk, eventloop);
    DFK_FREE(dfk, mainf);
    return dfk->dfk_errno;
  }

  dfk_fiber_name(scheduler, "scheduler");
  dfk_fiber_name(eventloop, "eventloop");
  dfk_fiber_name(mainf, "main");

  dfk->_eventloop = eventloop;

  /* Same format as in fiber.c */
#if DFK_NAMED_FIBERS
  DFK_DBG(dfk, "context switch {*} -> {scheduler}");
#else
  DFK_DBG(dfk, "context switch {*} -> {%p}", (void*) dfk->_scheduler);
#endif
  coro_transfer(&dfk->_comeback, &scheduler->_ctx);

  DFK_INFO(dfk, "work cycle {%p} done", (void*) dfk);
  return dfk_err_ok;
}

