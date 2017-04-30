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

  /**
   * Initial stack size for new coroutines
   *
   * @note default: #DFK_STACK_SIZE
   */
  size_t default_stack_size;

  int sys_errno;
  int dfk_errno;

  /**
   * @privatesection
   */

  struct dfk_coro_t* _current;
  struct dfk_coro_t* _scheduler;
  struct dfk_coro_t* _eventloop;
  struct dfk_coro_t* _terminator;
  sig_atomic_t _stopped;
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
 */
int dfk_work(dfk_t* dfk);

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

