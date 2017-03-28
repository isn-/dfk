/**
 * @file dfk/http/protocol.h
 * HTTP protocol implementation
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/core.h>
#include <dfk/tcp_socket.h>
#include <dfk/http/server.h>

/**
 * @addtogroup http
 * @{
 */

/**
 * HTTP protocol implementation
 * @private
 */
void dfk_http(dfk_coro_t* coro, dfk_tcp_socket_t* sock, dfk_http_t* http);

/** @} */

