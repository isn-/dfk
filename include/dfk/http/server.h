/**
 * @file dfk/http/server.h
 * HTTP server
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/config.h>
#include <dfk/core.h>
#include <dfk/sync.h>
#include <dfk/tcp_socket.h>
#include <dfk/http/request.h>
#include <dfk/http/response.h>
#include <dfk/internal/list.h>

/**
 * @addtogroup http
 * @{
 */

struct dfk_http_t;

typedef int (*dfk_http_handler)(dfk_userdata_t ud, struct dfk_http_t*, dfk_http_request_t*,
                                dfk_http_response_t*);

typedef struct dfk_http_t {
  /** @privatesection */
  dfk_list_hook_t _hook;
  dfk_tcp_socket_t _listensock;
  dfk_http_handler _handler;
  dfk_userdata_t _handler_ud;
  dfk_list_t _connections;
  /**
   * Server state
   *
   * Can be one of
   * @li 0 - initialized, idle
   * @li 1 - serving
   * @li 2 - stopping
   * @li 3 - stopped
   */
  int _state;

  /**
   * Protects _state
   */
  dfk_mutex_t _state_mut;

  /**
   * Notifies about _state changes
   */
  dfk_cond_t _state_cv;

  /** @publicsection */
  dfk_t* dfk;
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

int dfk_http_init(dfk_http_t* http, dfk_t* dfk);
int dfk_http_stop(dfk_http_t* http);
int dfk_http_free(dfk_http_t* http);

/**
 * Returns size of the dfk_http_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_http_sizeof(void);

int dfk_http_serve(dfk_http_t* http,
    const char* endpoint,
    uint16_t port,
    dfk_http_handler handler,
    dfk_userdata_t handler_ud);

/** @} */

