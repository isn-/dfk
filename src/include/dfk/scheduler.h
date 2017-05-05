/**
 * @file dfk/scheduler.h
 * Fiber scheduler
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/list.h>
#include <dfk/fiber.h>

/**
 * Scheduler object
 */
typedef struct dfk_scheduler_t {
  /**
   * Scheduler's own fiber
   */
  dfk_fiber_t* fiber;
  dfk_list_t pending;
  dfk_list_t iowait;
  dfk_list_t terminated;
} dfk_scheduler_t;

void dfk__scheduler_init(dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);
int dfk__scheduler(dfk_scheduler_t* scheduler);

/**
 * Scheduler entry point - main loop
 *
 * @param arg a pointer to main fiber
 */
void dfk__scheduler_loop(dfk_fiber_t* fiber, void* arg);

void dfk__schedule(dfk_t* dfk, dfk_fiber_t* fiber);
void dfk__terminate(dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);

