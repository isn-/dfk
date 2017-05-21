/**
 * @file dfk/scheduler.h
 * Contains declaration of the scheduler interface
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/fiber.h>

/**
 * @par Writing new scheduler
 * To create a new scheduler one have to implement following functions:
 * - dfk__scheduler_main
 * - dfk__resume
 * - dfk__terminate
 * - dfk__yielded
 * - dfk__this_fiber
 * - dfk__suspend
 * - dfk__postpone
 */

/**
 * Scheduler entry point - main loop
 *
 * @todo Write a wrapper that properly handles "return from scheduler", see
 * scheduler.c:202
 *
 * @param arg  a pointer to main fiber (dfk_fiber_t*). Scheduler is responsible
 * for cleaning up main fiber using dfk__fiber_free.
 */
void dfk__scheduler_main(dfk_fiber_t* fiber, void* arg);

/**
 * Schedule fiber to the future execution
 */
void dfk__resume(struct dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);

/**
 * Fiber's exit point.
 */
void dfk__terminate(struct dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);

/**
 * Called each time pre-emptive context switch is to be performed
 */
void dfk__yielded(struct dfk_scheduler_t* scheduler, dfk_fiber_t* from,
    dfk_fiber_t* to);

/**
 * Returns a handle of the current fiber
 */
dfk_fiber_t* dfk__this_fiber(struct dfk_scheduler_t* scheduler);

/**
 * Suspend current fiber.
 *
 * Implemented as yielding to scheduler fiber without queuing current fiber.
 * Fiber will never be executed without manual re-scheduling via dfk__resume.
 */
void dfk__suspend(struct dfk_scheduler_t* scheduler);

/**
 * Suspend current fiber because it is waiting for I/O operation to complete.
 *
 * @pre scheduler != NULL
 */
void dfk__iosuspend(struct dfk_scheduler_t* scheduler);

/**
 * Schedule fiber previously suspended for I/O to the future execution.
 *
 * @pre scheduler != NULL
 */
void dfk__ioresume(struct dfk_scheduler_t* scheduler, dfk_fiber_t* fiber);

/**
 * Postpone current fiber.
 *
 * Use to yield to another fiber determined by the scheduler.
 *
 * @warning Do not call directly, use #DFK_POSTPONE macro instead
 */
void dfk__postpone(struct dfk_scheduler_t* scheduler);

