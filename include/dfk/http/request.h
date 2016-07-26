/**
 * @file dfk/http/request.h
 * HTTP request
 *
 * @copyright
 * Copyright (c) 2016, Stanislav Ivochkin. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

