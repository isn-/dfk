/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <dfk/fiber.h>
#include <dfk/internal/fiber.h>
#include <dfk/error.h>
#include <dfk/internal.h>
#include <dfk/internal/malloc.h>
#include <dfk/scheduler.h>

#if DFK_STACK_GUARD_SIZE
#if DFK_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#if DFK_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#endif

#if DFK_NAMED_FIBERS
#include <stdarg.h>
#endif

#if DFK_VALGRIND
#include <valgrind/valgrind.h>
#endif

/**
 * An entry point for all fibers
 */
static void dfk__fiber_main(void* arg)
{
  dfk_fiber_t* fiber = (dfk_fiber_t*) arg;
  fiber->_ep(fiber, fiber->_arg);
  /*
   * A fiber will never return from dfk__terminate, therefore
   * end of the function is excluded from lcov coverage report
   */
  dfk__terminate(fiber->dfk->_scheduler, fiber);
} /* LCOV_EXCL_LINE */

dfk_fiber_t* dfk__run(dfk_t* dfk, void (*ep)(dfk_fiber_t*, void*),
    void* arg, size_t argsize)
{
  assert(dfk);
  assert(ep);
  dfk_fiber_t* fiber = dfk__malloc(dfk, dfk->default_stack_size);
  if (!fiber) {
    dfk->dfk_errno = dfk_err_nomem;
    return NULL;
  }
  char* stack_base = (char*) fiber + sizeof(dfk_fiber_t);
  char* stack_end = (char*) fiber + dfk->default_stack_size;
  if (argsize) {
    memcpy(stack_base, arg, argsize);
    fiber->_arg = stack_base;
    stack_base += argsize;
  } else {
    fiber->_arg = arg;
  }

#if DFK_STACK_GUARD_SIZE
  /** @todo check return code */
  mprotect(stack_base, DFK_STACK_GUARD_SIZE, PROT_NONE);
  stack_base += DFK_STACK_GUARD_SIZE;
#endif

  /* Align stack_base */
  if ((ptrdiff_t) stack_base % DFK_STACK_ALIGNMENT) {
    size_t padding = DFK_STACK_ALIGNMENT -
      ((ptrdiff_t) stack_base % DFK_STACK_ALIGNMENT);
    DFK_DBG(dfk, "stack pointer %p is not aligned to %d bytes, "
        "adjust by %lu bytes", (void*) stack_base, DFK_STACK_ALIGNMENT,
        (unsigned long) padding);
    stack_base += padding;
  }

#if DFK_VALGRIND
  fiber->_stack_id = VALGRIND_STACK_REGISTER(stack_base, stack_end);
#endif

  fiber->dfk = dfk;
  dfk_list_hook_init(&fiber->_hook);
  fiber->_ep = ep;

  dfk_fiber_name(fiber, "%p", (void*) fiber);

  assert(stack_end > stack_base);
  size_t stack_size = stack_end - stack_base;
  DFK_INFO(dfk, "stack %p (%lu bytes) = {%p}",
      (void*) stack_base, (unsigned long) stack_size, (void*) fiber);
  coro_create(&fiber->_ctx, dfk__fiber_main, fiber, stack_base, stack_size);
  return fiber;
}

void dfk__fiber_free(dfk_t* dfk, dfk_fiber_t* fiber)
{
#if DFK_VALGRIND
  char* stack_base = (char*) fiber + sizeof(dfk_fiber_t);
  stack_base += DFK_STACK_ALIGNMENT - 1;
  stack_base -= ((ptrdiff_t) stack_base) % DFK_STACK_ALIGNMENT;
  VALGRIND_STACK_DEREGISTER(stack_base);
#endif
  dfk__free(dfk, fiber);
}

dfk_fiber_t* dfk_run(dfk_t* dfk, void (*ep)(dfk_fiber_t*, void*),
    void* arg, size_t argsize)
{
  dfk_fiber_t* fiber = dfk__run(dfk, ep, arg, argsize);
  if (!fiber) {
    return NULL;
  }
  dfk__resume(dfk->_scheduler, fiber);
  return fiber;
}

void dfk_fiber_name(dfk_fiber_t* fiber, const char* fmt, ...)
{
  assert(fiber);
  assert(fmt);
#if DFK_NAMED_FIBERS
  va_list args;
  va_start(args, fmt);
  snprintf(fiber->_name, sizeof(fiber->_name), fmt, args);
  DFK_DBG(fiber->dfk, "{%p} is now known as %s", (void*) fiber, fiber->_name);
  va_end(args);
#else
  DFK_UNUSED(fiber);
  DFK_UNUSED(fmt);
#endif
}

void dfk_yield(dfk_fiber_t* from, dfk_fiber_t* to)
{
  assert(from);
  assert(to);
#if DFK_NAMED_FIBERS
  DFK_DBG(from->dfk, "context switch {%s} -> {%s}", from->_name, to->_name);
#else
  DFK_DBG((from ? from : to)->dfk, "context switch {%p} -> {%p}",
      (void*) from, (void*) to);
#endif
  /* Notify scheduler about manual context switch. */
  dfk__yield(from->dfk->_scheduler, from, to);
  coro_transfer(&from->_ctx, &to->_ctx);
}

size_t dfk_fiber_sizeof(void)
{
  return sizeof(dfk_fiber_t);
}

