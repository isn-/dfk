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
#include <dfk/tcp_socket.h>
#include <dfk/strmap.h>
#if DFK_MOCKS
#include <dfk/sponge.h>
#endif
#include <dfk/http/constants.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HTTP request type
 */
typedef struct dfk_http_request_t {
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
  /**
   * Negative values means content length is unknown.
   */
  int64_t content_length;
  unsigned int keepalive : 1;
  unsigned int chunked : 1;
  dfk_strmap_t headers;
  dfk_strmap_t arguments;

  /**
   * @privatesection
   */
  struct dfk_arena_t* _connection_arena;
  struct dfk_arena_t* _request_arena;
  dfk_tcp_socket_t* _socket;
  dfk_list_t _buffers;
  dfk_buf_t _remainder;
  size_t _body_nread;
  http_parser _parser;

#if DFK_MOCKS
  int _socket_mocked : 1;
  dfk__sponge_t* _socket_mock;
#endif

} dfk_http_request_t;

/**
 * Returns size of the dfk_http_request_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_http_request_sizeof(void);

/**
 * Read request body
 */
ssize_t dfk_http_request_read(dfk_http_request_t* req, char* buf, size_t size);

/**
 * Read request body into multiple buffers at once
 */
ssize_t dfk_http_request_readv(dfk_http_request_t* req,
    dfk_iovec_t* iov, size_t niov);

#ifdef __cplusplus
}
#endif

