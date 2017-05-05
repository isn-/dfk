/**
 * @file dfk/fiber.h
 * Contains a definition of dfk_fiber_t and related routines.
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/context.h>
#include <dfk/list.h>
#include <dfk/misc.h>
#include <dfk/thirdparty/libcoro/coro.h>

/**
 * Fiber - a lightweight userspace thread
 */
typedef struct dfk_fiber_t {
  /**
   * @privatesection
   */

  /**
   * Needed for scheduler - it keeps lists of ready/pending fibers.
   */
  dfk_list_hook_t _hook;

  struct coro_context _ctx;

  /** Entry point of the fiber */
  void (*_ep)(struct dfk_fiber_t*, void*);

  /** Argument provided to the entry point */
  void* _arg;
#if DFK_NAMED_FIBERS
  char _name[DFK_FIBER_NAME_LENGTH];
#endif
#if DFK_VALGRIND
  int _stack_id;
#endif

  /**
   * @publicsection
   */

  dfk_t* dfk;
  dfk_userdata_t user;
} dfk_fiber_t;

/**
 * Start a new fiber
 *
 * Resources allocated to fiber will be automatically released upon completion.
 *
 * @param argsize Size of the arg object. If argsize is set to non-zero value,
 * @p argsize bytes of memory pointed by @p arg will be copied onto the stack
 * of the newly created fiber and @p ep call will be provided with arg copy.
 * Set argsize to zero if arg is guaranteed to be accessible within spawned
 * fiber lifetime.
 */
dfk_fiber_t* dfk_run(dfk_t* dfk, void (*ep)(dfk_fiber_t*, void*),
    void* arg, size_t argsize);

/**
 * Set name of the fiber.
 *
 * If #DFK_NAMED_FIBERS is disabled, function returns dfk_err_ok and has
 * no effect.
 */
void dfk_fiber_name(dfk_fiber_t* fiber, const char* fmt, ...);

/**
 * Switch execution context to another fiber.
 *
 * A basement for preemtive multitasking.
 *
 * @note dfk_yield is a low-level function and is not recommended for everyday
 * usage. Prefer using sync module for higher-level fiber synchronization
 * techniques.
 */
void dfk_yield(dfk_fiber_t* from, dfk_fiber_t* to);

/**
 * Returns size of the dfk_fiber_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_fiber_sizeof(void);

