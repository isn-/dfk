/**
 * @file dfk/internal/http/response.h
 * HTTP response - private methods
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/http/response.h>

void dfk__http_response_init(dfk_http_response_t* resp, dfk_http_request_t* req,
    dfk_arena_t* request_arena, dfk_arena_t* connection_arena,
    dfk_tcp_socket_t* sock, int keepalive);

int dfk__http_response_flush_headers(dfk_http_response_t* resp);

