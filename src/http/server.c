/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/error.h>
#include <dfk/internal.h>
#include <dfk/http/server.h>
#include <dfk/http/protocol.h>

typedef struct serve_args {
  dfk_http_handler handler;
  dfk_userdata_t user;
} serve_args ;

static void dfk_http_connection(dfk_tcp_server_t* tcpserver, dfk_fiber_t* fiber,
    dfk_tcp_socket_t* sock, dfk_userdata_t user)
{
  serve_args* arg = (serve_args*) user.data;
  dfk_http_t* http = (dfk_http_t*) tcpserver; /** @bug */
  dfk__http_protocol(http, fiber, sock, arg->handler, arg->user);
}

void dfk_http_init(dfk_http_t* http, dfk_t* dfk)
{
  assert(http);
  assert(dfk);
  DFK_DBG(dfk, "{%p}", (void*) http);
  http->keepalive_requests = DFK_HTTP_KEEPALIVE_REQUESTS;
  http->header_max_size = DFK_HTTP_HEADER_MAX_SIZE;
  http->headers_buffer_size = DFK_HTTP_HEADERS_BUFFER_SIZE;
  http->headers_buffer_count = DFK_HTTP_HEADERS_BUFFER_COUNT;
  dfk_tcp_server_init(&http->_server, dfk);
}

int dfk_http_is_stopping(dfk_http_t* http)
{
  assert(http);
  return dfk_tcp_server_is_stopping(&http->_server);
}

int dfk_http_stop(dfk_http_t* http)
{
  assert(http);
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  int err = dfk_tcp_server_stop(&http->_server);
  if (err != dfk_err_ok) {
    return err;
  }
  return dfk_err_ok;
}

void dfk_http_free(dfk_http_t* http)
{
  assert(http);
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  dfk_tcp_server_free(&http->_server);
}

int dfk_http_serve(dfk_http_t* http,
    const char* endpoint,
    uint16_t port,
    size_t backlog,
    dfk_http_handler handler,
    dfk_userdata_t user)
{
  assert(http);
  assert(endpoint);
  assert(handler);
  assert(port);
  serve_args args = {
    .handler = handler,
    .user = user
  };
  return dfk_tcp_serve(&http->_server, endpoint, port, backlog,
      dfk_http_connection, (dfk_userdata_t) {.data = &args});
}

size_t dfk_http_sizeof(void)
{
  return sizeof(dfk_http_t);
}

