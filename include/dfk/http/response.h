/**
 * @file dfk/http/response.h
 * HTTP response
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <dfk/arena.h>
#include <dfk/strmap.h>
#include <dfk/tcp_socket.h>
#include <dfk/http/constants.h>
#include <dfk/http/request.h>
#if DFK_MOCKS
#include <dfk/sponge.h>
#endif

/**
 * HTTP response
 */
typedef struct dfk_http_response_t {
  /** @privatesection */
  dfk_arena_t* _request_arena;
  dfk_arena_t* _connection_arena;
  dfk_tcp_socket_t* _socket;

#if DFK_MOCKS
  int _socket_mocked;
  dfk__sponge_t* _socket_mock;
#endif

  int _headers_flushed : 1;

  /** @publicsection */
  struct dfk_http_t* http;
  unsigned short major_version;
  unsigned short minor_version;
  dfk_http_status_e status;
  size_t content_length;
  int chunked : 1;
  int keepalive : 1;
  dfk_strmap_t headers;
} dfk_http_response_t;


/**
 * Returns size of the dfk_http_response_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_http_response_sizeof(void);

ssize_t dfk_http_response_write(dfk_http_response_t* resp,
    char* buf, size_t nbytes);

ssize_t dfk_http_response_writev(dfk_http_response_t* resp,
    dfk_iovec_t* iov, size_t niov);

int dfk_http_response_set(dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen);

int dfk_http_response_set_copy(dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen);

int dfk_http_response_set_copy_name(dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen);

int dfk_http_response_set_copy_value(dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen);

int dfk_http_response_bset(dfk_http_response_t* resp,
    dfk_buf_t name, dfk_buf_t value);

int dfk_http_response_bset_copy(dfk_http_response_t* resp,
    dfk_buf_t name, dfk_buf_t value);

int dfk_http_response_bset_copy_name(dfk_http_response_t* resp,
    dfk_buf_t name, dfk_buf_t value);

int dfk_http_response_bset_copy_value(dfk_http_response_t* resp,
    dfk_buf_t name, dfk_buf_t value);

