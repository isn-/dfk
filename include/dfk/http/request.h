/**
 * @file dfk/http/request.h
 * HTTP request
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <http_parser.h>
#include <dfk/config.h>
#include <dfk/core.h>
#include <dfk/tcp_socket.h>
#include <dfk/strmap.h>
#include <dfk/internal/arena.h>
#include <dfk/http/constants.h>
#if DFK_MOCKS
#include <dfk/internal/sponge.h>
#endif

/**
 * @addtogroup http
 * @{
 */

/**
 * HTTP request type
 */
typedef struct dfk_http_request_t {
  /**
   * @privatesection
   */
  dfk_arena_t* _connection_arena;
  dfk_arena_t* _request_arena;
  dfk_tcp_socket_t* _sock;
  dfk_list_t _buffers;
  dfk_buf_t _remainder;
  size_t _body_nread;
  http_parser _parser;

#if DFK_MOCKS
  int _sock_mocked;
  dfk_sponge_t* _sock_mock;
#endif

  /**
   * @publicsection
   */

  struct dfk_http_t* http;

  unsigned short major_version;
  unsigned short minor_version;
  dfk_http_method_e method;
  dfk_buf_t url;
  dfk_buf_t path;
  dfk_buf_t query;
  dfk_buf_t fragment;
  dfk_buf_t host;
  dfk_buf_t user_agent;
  dfk_buf_t accept;
  dfk_buf_t content_type;
  uint64_t content_length;
  unsigned int keepalive : 1;
  unsigned int chunked : 1;
  dfk_strmap_t headers;
  dfk_strmap_t arguments;
} dfk_http_request_t;


/**
 * Initialize dfk_http_request_t
 * @private
 *
 * dfk_http_request_t objects are created and initialized within
 * dfk_http function, therefore this function is marked as private.
 */
int dfk_http_request_init(dfk_http_request_t* req, struct dfk_http_t* http,
                          dfk_arena_t* request_arena,
                          dfk_arena_t* connection_arena,
                          dfk_tcp_socket_t* sock);


/**
 * Cleanup resources allocated for dfk_http_request_t
 * @private
 *
 * dfk_http_request_t objects are created and free'd within
 * dfk_http function, therefore this function is marked as private.
 */
int dfk_http_request_free(dfk_http_request_t* req);

/**
 * Returns size of the dfk_http_request_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_http_request_sizeof(void);


/**
 * Read HTTP request headers.
 * @private
 *
 * Reads and parses url and headers.
 *
 * A part of request body, or even the next request within connection may
 * be read by this function. If so, unused data is stored in
 * dfk_http_request_t._remainder member and is accessible within
 * dfk_http_request_t lifetime.
 */
int dfk_http_request_read_headers(dfk_http_request_t* req);


/**
 * Read request body
 */
ssize_t dfk_http_request_read(dfk_http_request_t* req, char* buf, size_t size);


/**
 * Read request body into multiple buffers at once
 */
ssize_t dfk_http_request_readv(dfk_http_request_t* req, dfk_iovec_t* iov, size_t niov);


/** @} */

