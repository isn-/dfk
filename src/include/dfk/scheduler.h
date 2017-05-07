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
 *
 * To create a new scheduler one have to implement following functions:
 * - dfk__scheduler
 * - dfk__resume
 * - dfk__terminate
 * - dfk__this_fiber
 * - dfk__suspend
 * - dfk__postpone
 */
typedef struct dfk_scheduler_t {
  /**
   * Scheduler's own fiber
   */
  dfk_fiber_t* fiber;
  dfk_fiber_t* current;
  dfk_list_t pending;
  dfk_list_t iowait;
  dfk_list_t terminated;
} dfk_scheduler_t;

void dfk__scheduler_init(dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);

/**
 * Scheduler loop iteration
 */
int dfk__scheduler(dfk_scheduler_t* scheduler);

/**
 * Schedule fiber to the future execution
 */
void dfk__resume(dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);

/**
 * Fiber's exit point.
 */
void dfk__terminate(dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);

/**
 * Called each time pre-emptive context switch is to be performed
 */
void dfk__yield(dfk_scheduler_t* scheduler, dfk_fiber_t* from, dfk_fiber_t* to);

/**
 * Returns a handle of the current fiber
 */
dfk_fiber_t* dfk__this_fiber(dfk_scheduler_t* scheduler);

/**
 * Suspend current fiber.
 *
 * Implemented as yielding to scheduler fiber without queuing current fiber.
 * Fiber will never be executed without manual re-scheduling.
 */
void dfk__suspend(dfk_scheduler_t* scheduler);

/**
 * Postpone current fiber.
 *
 * Use to yield to another fiber determined by the scheduler.
 */
void dfk__postpone(dfk_scheduler_t* scheduler);

/**
 * Scheduler entry point - main loop
 *
 * @param arg a pointer to main fiber
 */
void dfk__scheduler_loop(dfk_fiber_t* fiber, void* arg);


