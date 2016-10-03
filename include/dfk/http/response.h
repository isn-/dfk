/**
 * @file dfk/http/response.h
 * HTTP response
 *
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#include <stddef.h>
#include <dfk/tcp_socket.h>
#include <dfk/strmap.h>
#include <dfk/http/constants.h>
#include <dfk/http/request.h>
#include <dfk/internal/arena.h>
#if DFK_MOCKS
#include <dfk/internal/sponge.h>
#endif

/**
 * @addtogroup http
 * @{
 */

/**
 * HTTP response
 */
typedef struct dfk_http_response_t {
  /** @privatesection */
  dfk_arena_t* _request_arena;
  dfk_arena_t* _connection_arena;
  dfk_tcp_socket_t* _sock;

#if DFK_MOCKS
  int _sock_mocked;
  dfk_sponge_t* _sock_mock;
#endif

  int _headers_flushed : 1;

  /** @publicsection */
  struct dfk_http_t* http;
  unsigned short major_version;
  unsigned short minor_version;
  dfk_http_status_e code;
  size_t content_length;
  int chunked : 1;
  int keepalive : 1;
  dfk_strmap_t headers;
} dfk_http_response_t;


int dfk_http_response_init(dfk_http_response_t* resp,
                           dfk_http_request_t* req,
                           dfk_arena_t* request_arena,
                           dfk_arena_t* connection_arena,
                           dfk_tcp_socket_t* sock,
                           int keepalive);

int dfk_http_response_free(dfk_http_response_t* resp);

int dfk_http_response_flush_headers(dfk_http_response_t* resp);

ssize_t dfk_http_response_write(dfk_http_response_t* resp, char* buf, size_t nbytes);
ssize_t dfk_http_response_writev(dfk_http_response_t* resp, dfk_iovec_t* iov, size_t niov);

int dfk_http_response_set(dfk_http_response_t* resp,
                          const char* name, size_t namelen,
                          const char* value, size_t valuelen);

int dfk_http_response_set_copy(dfk_http_response_t* resp,
                               const char* name, size_t namelen,
                               const char* value, size_t valuelen);

int dfk_http_response_set_copy_name(dfk_http_response_t* resp,
                                    const char* name, size_t namelen,
                                    const char* value, size_t valuelen);

int dfk_http_response_set_copy_value(dfk_http_response_t* resp,
                                     const char* name, size_t namelen,
                                     const char* value, size_t valuelen);

int dfk_http_response_bset(dfk_http_response_t* resp,
                           dfk_buf_t name, dfk_buf_t value);

int dfk_http_response_bset_copy(dfk_http_response_t* resp,
                                dfk_buf_t name, dfk_buf_t value);

int dfk_http_response_bset_copy_name(dfk_http_response_t* resp,
                                     dfk_buf_t name, dfk_buf_t value);

int dfk_http_response_bset_copy_value(dfk_http_response_t* resp,
                                      dfk_buf_t name, dfk_buf_t value);


/** @} */

