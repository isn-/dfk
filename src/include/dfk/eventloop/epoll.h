/**
 * @file dfk/eventloop/epoll.h
 * Contains epoll-based event loop implementation
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <sys/epoll.h>
#include <dfk/context.h>

#ifndef DFK_INCLUDE_EVENTLOOP_EPOLL_H_DIRECTLY
#error("Do not include this header directly, use <dfk/eventloop.h>")
#endif

typedef enum dfk_io_event_e {
  DFK_IO_IN = EPOLLIN,
  DFK_IO_OUT = EPOLLOUT,
  DFK_IO_ERR = EPOLLERR,
} dfk_io_event_e;

typedef struct dfk_eventloop_t {
  dfk_t* dfk;
  int fd;
} dfk_eventloop_t;

