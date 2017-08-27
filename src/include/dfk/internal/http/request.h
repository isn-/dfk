/**
 * @file dfk/internal/http/request.h
 * HTTP request - private methods
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/arena.h>
#include <dfk/tcp_socket.h>
#include <dfk/http/request.h>

/**
 * Initialize dfk_http_request_t
 *
 * dfk_http_request_t objects are created and initialized within
 * dfk_http function, therefore this function is marked as private.
 */
void dfk__http_request_init(dfk_http_request_t* req, struct dfk_http_t* http,
    dfk_arena_t* request_arena, dfk_arena_t* connection_arena,
    dfk_tcp_socket_t* sock);

/**
 * Cleanup resources allocated for dfk_http_request_t
 *
 * dfk_http_request_t objects are created and free'd within
 * dfk_http function, therefore this function is marked as private.
 */
void dfk__http_request_free(dfk_http_request_t* req);

/**
 * Read HTTP request headers.
 *
 * Reads and parses url and headers.
 *
 * A part of request body, or even the next request within connection may
 * be read by this function. If so, unused data is stored in
 * dfk_http_request_t._remainder member and is accessible within
 * dfk_http_request_t lifetime.
 */
int dfk__http_request_read_headers(dfk_http_request_t* req);

