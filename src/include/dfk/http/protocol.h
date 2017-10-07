/**
 * @file dfk/http/protocol.h
 * HTTP protocol implementation
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/fiber.h>
#include <dfk/tcp_socket.h>
#include <dfk/misc.h>
#include <dfk/http/server.h>

/**
 * HTTP protocol implementation
 * @private
 */
void dfk__http_protocol(dfk_http_t* http, dfk_fiber_t* fiber, dfk_tcp_socket_t* sock,
    dfk_http_handler handler, dfk_userdata_t user);

