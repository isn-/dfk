/**
 * @file dfk/loop.h
 * Event loop
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/core.h>

typedef struct dfk_loop_t {
  dfk_coro_t coro;
} dfk_loop_t;

int dfk_loop_init(dfk_loop_t* loop);
int dfk_loop_free(dfk_loop_t* loop);

