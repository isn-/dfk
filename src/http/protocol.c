/**
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

#include <assert.h>
#include <dfk/internal.h>
#include <dfk/http/protocol.h>
#include <dfk/http/request.h>
#include <dfk/http/response.h>


void dfk_http(dfk_coro_t* coro, dfk_tcp_socket_t* sock, dfk_http_t* http)
{
  DFK_UNUSED(coro);
  assert(sock);
  assert(http);
  dfk_t* dfk = coro->dfk;

  DFK_DBG(dfk, "{%p} initialize connection arena", (void*) sock);
  /* Arena for per-connection data */
  dfk_arena_t connection_arena;
  dfk_arena_init(&connection_arena, http->dfk);

  /* Requests processed within this connection */
  ssize_t nrequests = 0;
  int keepalive = 1;

  while (keepalive) {
    DFK_DBG(dfk, "{%p} initialize request arena", (void*) sock);
    /* Arena for per-request data */
    dfk_arena_t request_arena;
    dfk_arena_init(&request_arena, http->dfk);

    dfk_http_request_t req;
    /** @todo check return value */
    dfk_http_request_init(&req, http, &request_arena, &connection_arena, sock);

    if (dfk_http_request_read_headers(&req) != dfk_err_ok) {
      goto connection_broken;
    }

    DFK_DBG(http->dfk, "{%p} request parsing done, "
        "%s %.*s HTTP/%hu.%hu \"%.*s\"",
        (void*) http,
        http_method_str((enum http_method) req.method),
        (int) req.url.size, req.url.data,
        req.major_version,
        req.minor_version,
        (int) req.user_agent.size, req.user_agent.data);

    dfk_http_response_t resp;
    /** @todo check return value */
    dfk_http_response_init(&resp, http->dfk, &request_arena, &connection_arena, sock);

    DFK_DBG(http->dfk, "{%p} executing request handler", (void*) http);
    int hres = http->_handler(http, &req, &resp);
    DFK_INFO(http->dfk, "{%p} http handler returned %d", (void*) http, hres);
    if (hres != dfk_err_ok) {
      resp.code = DFK_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (http->keepalive_requests >= 0 && nrequests + 1 >= http->keepalive_requests) {
      DFK_INFO(http->dfk, "{%p} maximum number of keepalive requests (%llu) "
               "for connection {%p} has reached, close connection",
               (void*) http, (unsigned long long) http->keepalive_requests,
               (void*) sock);
      keepalive = 0;
    }

    dfk_buf_t connection = dfk_strmap_get(&resp.headers, DFK_HTTP_CONNECTION,
                                          sizeof(DFK_HTTP_CONNECTION));
    if (!strncmp(connection.data, "close", DFK_MIN(connection.size, 5))) {
      keepalive = 0;
    }

    /**
     * @todo Implement keepalive properly
     */
    keepalive = 0;

    dfk_http_response_flush_headers(&resp);

    ++nrequests;

connection_broken:
    /** @todo check for return value */
    dfk_http_response_free(&resp);
    dfk_http_request_free(&req);
    dfk_arena_free(&request_arena);
  }

  dfk_arena_free(&connection_arena);

  DFK_CALL_RVOID(http->dfk, dfk_tcp_socket_close(sock));
}

