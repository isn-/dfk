/**
 * @file dfk/sched.h
 * Coroutine scheduler
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 *
 */

#include <dfk/core.h>

/**
 * Scheduler object
 */
typedef struct dfk_sched_t {
  dfk_list_t pending;
  dfk_list_t iowait;
  dfk_list_t terminated;
} dfk_sched_t;

int dfk_sched_init(dfk_sched_t* sched);
int dfk_sched_free(dfk_sched_t* sched);

