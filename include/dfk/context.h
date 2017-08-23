/**
 * @file dfk/context.h
 * A context of the dfk library.
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <signal.h>
#include <dfk/misc.h>
#include <dfk/list.h>
#include <dfk/thirdparty/libcoro/coro.h>

/**
 * Library context
 *
 * All non-trivial operations require a valid context object. Context
 * defines:
 *  - memory management (malloc/free/realloc)
 *  - fiber managnement
 */
typedef struct dfk_t {
  /**
   * @publicsection
   */

  dfk_userdata_t user;

  /** User-provided memory allocation function */
  void* (*malloc) (struct dfk_t*, size_t);

  /** User-provided memory deallocation function */
  void (*free) (struct dfk_t*, void*);

  /** User-provided memory relocation function */
  void* (*realloc)(struct dfk_t*, void*, size_t);

  /** User-provided logging function */
  void (*log)(struct dfk_t*, int, const char*);

  /** Set to 1, if user-provided logging function is async-signal-safe
   *
   * @see http://man7.org/linux/man-pages/man7/signal-safety.7.html
   */
  int log_is_signal_safe;

  /**
   * Initial stack size for new fibers
   *
   * @note default: #DFK_STACK_SIZE
   */
  size_t default_stack_size;

  int sys_errno;
  int dfk_errno;

  /**
   * @privatesection
   */

  struct dfk_scheduler_t* _scheduler;
  struct dfk_eventloop_t* _eventloop;
  struct dfk_fiber_t* _terminator;

  /**
   * A coro object used as source for spawning first fiber.
   *
   * Last fiber standing (generally it is scheduler) should yield here to
   * indicate self termination.
   *
   * @todo move to internal function and declare as static variable
   */
  struct coro_context _comeback;
  sig_atomic_t _stopped;

  /**
   * A list of active tcp servers to wait for when dfk_stop() is called.
   */
  dfk_list_t _tcp_servers;
} dfk_t;

/**
 * Initialize dfk context with default settings.
 *
 * @pre dfk != NULL
 */
void dfk_init(dfk_t* dfk);

/**
 * Cleanup resources allocated for dfk context
 *
 * @pre dfk != NULL
 */
void dfk_free(dfk_t* dfk);

/**
 * Start dfk working cycle.
 *
 * A main fiber with entry point @ep is created and scheduled for execution.
 * @see dfk_run
 */
int dfk_work(dfk_t* dfk, void (*ep)(struct dfk_fiber_t*, void*),
    void* arg, size_t argsize);

/**
 * Stop dfk working cycle
 */
int dfk_stop(dfk_t* dfk);

/**
 * Returns size of the dfk_t structure.
 *
 * Useful for dynamic bindings' creators. dfk_t size may grow within minor
 * version update preserving backward compatibility, so it's considered a better
 * practice to allocate memory for dfk object dynamically.
 */
size_t dfk_sizeof(void);

