/**
 * @file dfk/eventloop/select.h
 * Contains select-based event loop implementation
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <sys/select.h>
#include <dfk/list.h>
#include <dfk/context.h>
#include <dfk/fiber.h>

#ifndef DFK_INCLUDE_EVENTLOOP_SELECT_H_DIRECTLY
#error("Do not include this header directly, use <dfk/eventloop.h>")
#endif

typedef enum dfk_io_event_e {
  DFK_IO_IN = 1,
  DFK_IO_OUT = 1 << 1,
  DFK_IO_ERR = 1 << 2,
} dfk_io_event_e;

typedef struct dfk_fdlist_element_t {
  dfk_list_hook_t hook;
  int fd;
  int events;
  dfk_fiber_t* yieldback;
} dfk_fdlist_element_t;

typedef struct dfk_eventloop_t {
  dfk_t* dfk;
  dfk_list_t fds;
} dfk_eventloop_t;
