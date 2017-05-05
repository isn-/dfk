/**
 * @file dfk/loop.h
 * Event loop
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/fiber.h>

/**
 * Event loop entry point
 */
void dfk__eventloop(dfk_fiber_t* fiber, void* arg);

