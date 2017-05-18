/**
 * @file dfk/eventloop.h
 * Contains declaration of the event loop interface
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/config.h>
#include <dfk/fiber.h>

/**
 * @par Writing a new event loop implementation
 * To create a new event loop implementation, one have to implement functions:
 * - dfk__eventloop_init
 * - dfk__eventloop_free
 * - dfk__eventloop_main
 * - dfk__io
 * define constants:
 * - DFK_IO_IN
 * - DFK_IO_OUT
 * - DFK_IO_ERR
 * and declare a structure
 * - dfk_eventloop_t
 */

#if DFK_EVENT_LOOP_EPOLL
/* A define to protect against direct inclusion of <dfk/eventloop/epoll.h> */
#define DFK_INCLUDE_EVENTLOOP_EPOLL_H_DIRECTLY
#include <dfk/eventloop/epoll.h>
#undef DFK_INCLUDE_EVENTLOOP_EPOLL_H_DIRECTLY
#endif

int dfk__eventloop_init(dfk_eventloop_t* loop, dfk_t* dfk);

int dfk__eventloop_free(dfk_eventloop_t* loop);

/**
 * Event loop entry point
 */
void dfk__eventloop_main(dfk_fiber_t* fiber, void* arg);

/**
 * Do possibly blocking input-ouput operation
 *
 * This call enqueues IO request and suspends current fiber until
 * result arrives.
 */
int dfk__io(dfk_eventloop_t* loop, int socket, int events);

/**
 * Writes string representation of the event set events to the buffer
 *
 * Format example: OUT|ERR
 */
size_t dfk__io_events_to_str(int events, char* buf, size_t buflen);

