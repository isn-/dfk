/**
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <http_parser.h>
#include <dfk.h>
#include <dfk/internal.h>

int dfk_http_init(dfk_http_t* http, dfk_t* dfk)
{
  if (!http || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  http->dfk = dfk;
  return dfk_tcp_socket_init(&http->_.listensock, dfk);
}


int dfk_http_free(dfk_http_t* http)
{
  if (!http) {
    return dfk_err_badarg;
  }
  return dfk_tcp_socket_free(&http->_.listensock);
}


static int dfk__http_req_init(dfk_http_req_t* req)
{
  DFK_UNUSED(req);
  return dfk_err_ok;
}


static int dfk__http_req_free(dfk_http_req_t* req)
{
  DFK_UNUSED(req);
  return dfk_err_ok;
}


static int dfk__http_resp_init(dfk_http_resp_t* resp)
{
  DFK_UNUSED(resp);
  return dfk_err_ok;
}


static int dfk__http_resp_free(dfk_http_resp_t* resp)
{
  DFK_UNUSED(resp);
  return dfk_err_ok;
}


typedef struct dfk__http_parser_data_t {
  dfk_t* dfk;
  dfk_http_req_t* req;
  int done;
} dfk__http_parser_data_t;


static void dfk__http_parser_data_init(dfk__http_parser_data_t* pdata, dfk_t* dfk, dfk_http_req_t* req)
{
  pdata->dfk = dfk;
  pdata->req = req;
  pdata->done = 0;
}


static void dfk__http_parser_data_free(dfk__http_parser_data_t* pdata)
{
  DFK_UNUSED(pdata);
}


static int dfk__http_on_message_begin(http_parser* parser)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  return 0;
}


static int dfk__http_on_url(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  DFK_UNUSED(at);
  DFK_UNUSED(size);
  return 0;
}


static int dfk__http_on_status(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  DFK_UNUSED(at);
  DFK_UNUSED(size);
  return 0;
}


static int dfk__http_on_header_field(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  DFK_UNUSED(at);
  DFK_UNUSED(size);
  return 0;
}


static int dfk__http_on_header_value(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  DFK_UNUSED(at);
  DFK_UNUSED(size);
  return 0;
}


static int dfk__http_on_headers_complete(http_parser* parser)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  return 0;
}


static int dfk__http_on_body(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  DFK_UNUSED(at);
  DFK_UNUSED(size);
  return 0;
}


static int dfk__http_on_message_complete(http_parser* parser)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  p->done = 1;
  return 0;
}


static int dfk__http_on_chunk_header(http_parser* parser)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  return 0;
}


static int dfk__http_on_chunk_complete(http_parser* parser)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  return 0;
}


static void dfk__http(dfk_coro_t* coro, dfk_tcp_socket_t* sock, void* p)
{
  dfk_http_t* http = (dfk_http_t*) p;
  http_parser parser;
  http_parser_settings parser_settings;
  dfk_http_req_t req;
  dfk_http_resp_t resp;
  char buf[DFK_HTTP_HEADERS_BUFFER] = {0};
  dfk__http_parser_data_t pdata;

  assert(http);
  dfk__http_req_init(&req);
  dfk__http_resp_init(&resp);
  dfk__http_parser_data_init(&pdata, coro->dfk, &req);

  http_parser_init(&parser, HTTP_REQUEST);
  parser.data = &pdata;

  http_parser_settings_init(&parser_settings);
  parser_settings.on_message_begin = dfk__http_on_message_begin;
  parser_settings.on_url = dfk__http_on_url;
  parser_settings.on_status = dfk__http_on_status;
  parser_settings.on_header_field = dfk__http_on_header_field;
  parser_settings.on_header_value = dfk__http_on_header_value;
  parser_settings.on_headers_complete = dfk__http_on_headers_complete;
  parser_settings.on_body = dfk__http_on_body;
  parser_settings.on_message_complete = dfk__http_on_message_complete;
  parser_settings.on_chunk_header = dfk__http_on_chunk_header;
  parser_settings.on_chunk_complete = dfk__http_on_chunk_complete;

  while (!pdata.done) {
    ssize_t nread = dfk_tcp_socket_read(sock, buf, sizeof(buf));
    if (nread < 0) {
      goto connection_broken;
    }
    DFK_DBG(http->dfk, "{%p} execute parser", (void*) http);
    http_parser_execute(&parser, &parser_settings, buf, nread);
    DFK_DBG(http->dfk, "{%p} parser finished", (void*) http);
    if (pdata.done) {
      char respbuf[19] = {0};
      http->_.handler(http->dfk, &req, &resp);
      snprintf(respbuf, sizeof(respbuf), "HTTP/1.0 %3d OK\r\n", resp.code);
      /* @todo remove it */
      dfk_tcp_socket_write(sock, respbuf, sizeof(respbuf) - 1);
      dfk__http_req_free(&req);
      dfk__http_resp_free(&resp);
    }
  }

  dfk__http_parser_data_free(&pdata);

connection_broken:
  DFK_CALL_RVOID(http->dfk, dfk_tcp_socket_close(sock));
}


int dfk_http_serve(dfk_http_t* http,
    const char* endpoint,
    uint16_t port,
    dfk_http_handler handler)
{
  if (!http || !endpoint || !handler || !port) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p} serving at %s:%u", (void*) http, endpoint, port);
  http->_.handler = handler;
  return dfk_tcp_socket_listen(&http->_.listensock, endpoint, port, dfk__http, http, 0);
}

