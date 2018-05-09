/**
 * @file dfk/http/server.h
 * HTTP server
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <dfk/context.h>
#include <dfk/list.h>
#include <dfk/tcp_server.h>
#include <dfk/http/request.h>
#include <dfk/http/response.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dfk_http_t {
  union {
    /**
     * @public
     */
    dfk_t* dfk;
    /**
     * @private
     */
    dfk_tcp_server_t _server;
  };
  /** @privatesection */
  dfk_list_hook_t _hook;

  /** @publicsection */
  dfk_userdata_t user;

  /**
   * Maximum number of requests for a single keepalive connection.
   *
   * Negative values means no limit.
   * @note default: #DFK_HTTP_KEEPALIVE_REQUESTS
   */
  ssize_t keepalive_requests;

  /**
   * Size of the buffer allocated for HTTP header parsing.
   *
   * @note default: #DFK_HTTP_HEADERS_BUFFER_SIZE
   */
  size_t headers_buffer_size;

  /**
   * Maximum number of buffers of size #DFK_HTTP_HEADERS_BUFFER_SIZE
   * consumed by HTTP request parser.
   *
   * @note default: #DFK_HTTP_HEADERS_BUFFER_COUNT
   */
  size_t headers_buffer_count;

  /**
   * Limit of the individual HTTP header line - url, "field: value".
   *
   * @note default: #DFK_HTTP_HEADER_MAX_SIZE
   */
  size_t header_max_size;
} dfk_http_t;

void dfk_http_init(dfk_http_t* http, dfk_t* dfk);
void dfk_http_free(dfk_http_t* http);

typedef int (*dfk_http_handler)(dfk_http_t*,
    dfk_http_request_t*, dfk_http_response_t*, dfk_userdata_t ud);

/**
 *
 * @todo Think about socket lingering
 * @todo Think about proper socket closing
 */
int dfk_http_serve(dfk_http_t* http,
    const char* endpoint,
    uint16_t port,
    size_t backlog,
    dfk_http_handler handler,
    dfk_userdata_t handler_ud);

int dfk_http_is_stopping(dfk_http_t* http);
int dfk_http_stop(dfk_http_t* http);

/**
 * Returns size of the dfk_http_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_http_sizeof(void);

#ifdef __cplusplus
}
#endif

