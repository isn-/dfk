/**
 * @file dfk/http/response.h
 * HTTP response
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
#include <stddef.h>
#include <dfk/tcp_socket.h>
#include <dfk/http/constants.h>
#include <dfk/http/header.h>
#include <dfk/internal/arena.h>
#include <dfk/internal/avltree.h>
#if DFK_MOCKS
#include <dfk/internal/sponge.h>
#endif


/**
 * HTTP response
 */
typedef struct dfk_http_response_t {
  /** @privatesection */
  dfk_arena_t* _request_arena;
  dfk_arena_t* _connection_arena;
  dfk_tcp_socket_t* _sock;
  dfk_avltree_t _headers;

#if DFK_MOCKS
  int _sock_mocked;
  dfk_sponge_t* _sock_mock;
#endif

  int _headers_flushed : 1;

  /** @publicsection */
  dfk_t* dfk;
  unsigned short major_version;
  unsigned short minor_version;
  dfk_http_status_e code;
  size_t content_length;
  int chunked : 1;
} dfk_http_response_t;


void dfk__http_response_init(dfk_http_response_t* resp, dfk_t* dfk,
                             dfk_arena_t* request_arena,
                             dfk_arena_t* connection_arena,
                             dfk_tcp_socket_t* sock);


void dfk__http_response_free(dfk_http_response_t* resp);


int dfk__http_response_flush_headers(dfk_http_response_t* resp);


int dfk__http_response_flush(dfk_http_response_t* resp);


dfk_buf_t dfk_http_response_get(struct dfk_http_response_t* resp,
                                const char* name, size_t namesize);


int dfk_http_response_headers_begin(struct dfk_http_response_t* resp,
                                    dfk_http_header_it* it);


ssize_t dfk_http_write(dfk_http_response_t* resp, char* buf, size_t nbytes);


ssize_t dfk_http_writev(dfk_http_response_t* resp, dfk_iovec_t* iov, size_t niov);


int dfk_http_set(dfk_http_response_t* resp, const char* name, size_t namesize,
                 const char* value, size_t valuesize);


int dfk_http_set_copy(dfk_http_response_t* resp, const char* name,
                      size_t namesize, const char* value, size_t valuesize);


int dfk_http_set_copy_name(dfk_http_response_t* resp, const char* name,
                           size_t namesize, const char* value,
                           size_t valuesize);


int dfk_http_set_copy_value(dfk_http_response_t* resp, const char* name,
                            size_t namesize, const char* value,
                            size_t valuesize);

