/**
 * @file dfk/sync.h
 * Synchronization primitives
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/config.h>
#include <dfk/core.h>
#include <dfk/internal/list.h>


/**
 * @defgroup sync sync
 * Synchronization primitives
 * @addtogroup sync
 * @{
 */

typedef struct dfk_mutex_t {
  /** @private */
  dfk_list_t _waitqueue;
  /** @private */
  dfk_coro_t* _owner;
  dfk_t* dfk;
} dfk_mutex_t;

int dfk_mutex_init(dfk_mutex_t* mutex, dfk_t* dfk);
int dfk_mutex_free(dfk_mutex_t* mutex);
int dfk_mutex_lock(dfk_mutex_t* mutex);
int dfk_mutex_unlock(dfk_mutex_t* mutex);
int dfk_mutex_trylock(dfk_mutex_t* mutex);


typedef struct dfk_cond_t {
  /** @private */
  dfk_list_t _waitqueue;
  dfk_t* dfk;
} dfk_cond_t;

int dfk_cond_init(dfk_cond_t* cond, dfk_t* dfk);
int dfk_cond_free(dfk_cond_t* cond);
int dfk_cond_wait(dfk_cond_t* cond, dfk_mutex_t* mutex);
int dfk_cond_signal(dfk_cond_t* cond);
int dfk_cond_broadcast(dfk_cond_t* cond);


typedef struct dfk_event_t {
  /** @private */
  dfk_coro_t* _awaiting;
  dfk_t* dfk;
} dfk_event_t;

int dfk_event_init(dfk_event_t* event, dfk_t* dfk);
int dfk_event_free(dfk_event_t* event);
int dfk_event_wait(dfk_event_t* event);
int dfk_event_signal(dfk_event_t* event);

/** @} */

