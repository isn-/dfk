/**
 * @file dfk/mutex.h
 * Contains a definition of dfk_mutex_t and related routines.
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/list.h>
#include <dfk/context.h>
#include <dfk/fiber.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * A recursive mutex
 */
typedef struct dfk_mutex_t {
  /**
   * @warning Readonly
   * @public
   */
  dfk_t* dfk;

  /**
   * A list of fibers waiting for the locked mutex
   *
   * @private
   */
  dfk_list_t _waitqueue;

  /**
   * A fiber that have acquired the mutex
   *
   * @note We can not get rid of this data member and assume that first element
   * of the _waitqueue is owner of the mutex. dfk_fiber_t has only one
   * dfk_list_hook_t member, so it can be contained in the single list at once.
   * Fiber can acquire a mutex and then call dfk__postpone (move to the list of
   * pending fibers) - in this case fiber will be contained in two lists
   * simultaneously.
   *
   * @private
   */
  dfk_fiber_t* _owner;
} dfk_mutex_t;

void dfk_mutex_init(dfk_mutex_t* mutex, dfk_t* dfk);
void dfk_mutex_free(dfk_mutex_t* mutex);
void dfk_mutex_lock(dfk_mutex_t* mutex);
void dfk_mutex_unlock(dfk_mutex_t* mutex);
int dfk_mutex_trylock(dfk_mutex_t* mutex);

#ifdef __cplusplus
}
#endif

