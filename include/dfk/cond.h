/**
 * @file dfk/cond.h
 * Contains a definition of dfk_cond_t and related routines.
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/list.h>
#include <dfk/context.h>
#include <dfk/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Condition variable
 *
 * @note Wait-invoke ordering is guaranteed in contrast to `pthread_cond_t'.
 */
typedef struct dfk_cond_t {
  /**
   * @warning Readonly
   * @public
   */
  dfk_t* dfk;

  /**
   * A list of fibers waiting for condition variable
   * @private
   */
  dfk_list_t _waitqueue;
} dfk_cond_t;

void dfk_cond_init(dfk_cond_t* cond, dfk_t* dfk);
void dfk_cond_free(dfk_cond_t* cond);
void dfk_cond_wait(dfk_cond_t* cond, dfk_mutex_t* mutex);
void dfk_cond_signal(dfk_cond_t* cond);
void dfk_cond_broadcast(dfk_cond_t* cond);

#ifdef __cplusplus
}
#endif

