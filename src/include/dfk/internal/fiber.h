/**
 * @file dfk/internal/fiber.h
 * Contains private functions to deal with dfk_fiber_t.
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/fiber.h>

/**
 * Same as dfk_run(), but do not schedule fiber for execution.
 *
 * Used in dfk_work to create system fibers - scheduler, eventloop, main.
 */
dfk_fiber_t* dfk__run(dfk_t* dfk, void (*ep)(dfk_fiber_t*, void*),
    void* arg, size_t argsize);

/**
 * Release resources allocated for the fiber
 */
void dfk__fiber_free(dfk_t* dfk, dfk_fiber_t* fiber);

