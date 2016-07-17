/**
 * @file dfk/http/server.h
 * HTTP server
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
#include <dfk/http/request.h>
#include <dfk/http/response.h>
#include <dfk/internal/list.h>


struct dfk_http_t;
typedef int (*dfk_http_handler)(struct dfk_http_t*, dfk_http_request_t*,
                                dfk_http_response_t*);


typedef struct dfk_http_t {
  /** @privatesection */
  dfk_list_hook_t _hook;
  dfk_tcp_socket_t _listensock;
  dfk_http_handler _handler;
  dfk_list_t _connections;

  /** @publicsection */
  dfk_t* dfk;
  dfk_userdata_t user;

  /**
   * Maximum number of requests for a single keepalive connection
   * @note default: 100
   */
  size_t keepalive_requests;
} dfk_http_t;


int dfk_http_init(dfk_http_t* http, dfk_t* dfk);
int dfk_http_stop(dfk_http_t* http);
int dfk_http_free(dfk_http_t* http);
int dfk_http_serve(dfk_http_t* http,
    const char* endpoint,
    uint16_t port,
    dfk_http_handler handler);

