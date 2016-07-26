/**
 * @file dfk/http/request.h
 * HTTP request
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
#include <dfk/config.h>
#include <dfk/core.h>
#include <dfk/tcp_socket.h>
#include <dfk/internal/avltree.h>
#include <dfk/internal/arena.h>
#include <dfk/http/constants.h>
#include <dfk/http/header.h>
#if DFK_MOCKS
#include <dfk/internal/sponge.h>
#endif


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
  dfk_avltree_t _headers; /* contains dfk_http_header_t */
  dfk_avltree_t _arguments; /* contains dfk_http_argument_t */
  dfk_buf_t _bodypart;
  size_t _body_bytes_nread;
  int _headers_done;

#if DFK_MOCKS
  int _sock_mocked;
  dfk_sponge_t* _sock_mock;
#endif

  /**
   * @publicsection
   */

  dfk_t* dfk;

  unsigned short major_version;
  unsigned short minor_version;
  dfk_http_method_e method;
  dfk_buf_t url;
  dfk_buf_t user_agent;
  dfk_buf_t host;
  dfk_buf_t accept;
  dfk_buf_t content_type;
  uint64_t content_length;
} dfk_http_request_t;


void dfk__http_request_init(dfk_http_request_t* req, dfk_t* dfk,
                            dfk_arena_t* request_arena,
                            dfk_arena_t* connection_arena,
                            dfk_tcp_socket_t* sock);

void dfk__http_request_free(dfk_http_request_t* req);


ssize_t dfk_http_read(dfk_http_request_t* req, char* buf, size_t size);
ssize_t dfk_http_readv(dfk_http_request_t* req, dfk_iovec_t* iov, size_t niov);

dfk_buf_t dfk_http_request_get(struct dfk_http_request_t* req,
                               const char* name, size_t namesize);

int dfk_http_request_headers_begin(struct dfk_http_request_t* req,
                                   dfk_http_header_it* it);


typedef dfk_http_header_t dfk_http_arg_t;
typedef dfk_http_header_it dfk_http_arg_it;

dfk_buf_t dfk_http_getarg(dfk_http_request_t* req, const char* name, size_t namesize);
int dfk_http_args_begin(dfk_http_request_t* req, dfk_http_arg_it* it);
int dfk_http_args_next(dfk_http_arg_it* it);
int dfk_http_args_end(dfk_http_arg_it* it);

