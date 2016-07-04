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


typedef struct dfk__http_header_t {
  dfk_avltree_hook_t hook;
  dfk_buf_t name;
  dfk_buf_t value;
} dfk__http_header_t;


static void dfk__http_header_init(dfk__http_header_t* header)
{
  assert(header);
  dfk_avltree_hook_init(&header->hook);
  header->name = (dfk_buf_t) {NULL, 0};
  header->value = (dfk_buf_t) {NULL, 0};
}


static int dfk__http_header_lookup_cmp(dfk_avltree_hook_t* l, void* r)
{
  dfk__http_header_t* lh = (dfk__http_header_t*) l;
  dfk_buf_t* rh = (dfk_buf_t*) r;
  size_t tocmp = DFK_MIN(lh->name.size, rh->size);
  int res = strncmp(lh->name.data, rh->data, tocmp);
  if (res) {
    return res;
  } else {
    return lh->name.size > rh->size;
  }
  return res;
}


static int dfk__http_header_cmp(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r)
{
  return dfk__http_header_lookup_cmp(l, &((dfk__http_header_t*) r)->name);
}


typedef dfk__http_header_t dfk_http_argument_t;


static void dfk__http_req_init(dfk_http_req_t* req, dfk_arena_t* arena, dfk_tcp_socket_t* sock)
{
  assert(req);
  assert(arena);
  assert(sock);
  req->_.arena = arena;
  req->_.sock = sock;
  dfk_avltree_init(&req->_.headers, dfk__http_header_cmp);
  dfk_avltree_init(&req->_.arguments, dfk__http_header_cmp);
  req->_.bodypart = (dfk_buf_t) {NULL, 0};
  req->url = (dfk_buf_t) {NULL, 0};
  req->user_agent = (dfk_buf_t) {NULL, 0};
  req->host = (dfk_buf_t) {NULL, 0};
  req->content_type = (dfk_buf_t) {NULL, 0};
  req->content_length = 0;
}


static void dfk__http_req_free(dfk_http_req_t* req)
{
  dfk_avltree_free(&req->_.arguments);
  dfk_avltree_free(&req->_.headers);
}


static void dfk__http_resp_init(dfk_http_resp_t* resp, dfk_arena_t* arena, dfk_tcp_socket_t* sock)
{
  assert(resp);
  assert(arena);
  assert(sock);
  resp->_.arena = arena;
  resp->_.sock = sock;
  dfk_avltree_init(&resp->_.headers, dfk__http_header_cmp);
}


static void dfk__http_resp_free(dfk_http_resp_t* resp)
{
  assert(resp);
  DFK_UNUSED(resp);
}


typedef struct dfk__http_parser_data_t {
  dfk_t* dfk;
  dfk_http_req_t* req;
  dfk__http_header_t* cheader; /* current header */
  int done;
} dfk__http_parser_data_t;


static void dfk__http_parser_data_init(dfk__http_parser_data_t* pdata, dfk_t* dfk, dfk_http_req_t* req)
{
  pdata->dfk = dfk;
  pdata->req = req;
  pdata->cheader = NULL;
  pdata->done = 0;
}


static void dfk__http_parser_data_free(dfk__http_parser_data_t* pdata)
{
  DFK_UNUSED(pdata);
}


static void dfk__buf_append(dfk_buf_t* to, const char* data, size_t size)
{
  assert(to);
  if (to->data) {
    assert(to->data + to->size == data);
    to->size += size;
  } else {
    to->data = (char*) data;
    to->size = size;
  }
}


static const char* dfk__http_reason_phrase(dfk_http_status_e status)
{
  switch (status) {
    case DFK_HTTP_CONTINUE: return "Continue";
    case DFK_HTTP_SWITCHING_PROTOCOLS: return "Switching Protocols";
    case DFK_HTTP_PROCESSING: return "Processing";
    case DFK_HTTP_OK: return "OK";
    case DFK_HTTP_CREATED: return "Created";
    case DFK_HTTP_ACCEPTED: return "Accepted";
    case DFK_HTTP_NON_AUTHORITATIVE_INFORMATION: return "Non Authoritative information";
    case DFK_HTTP_NO_CONTENT: return "No Content";
    case DFK_HTTP_RESET_CONTENT: return "Reset Content";
    case DFK_HTTP_PARTIAL_CONTENT: return "Partial Content";
    case DFK_HTTP_MULTI_STATUS: return "Multi Status";
    case DFK_HTTP_ALREADY_REPORTED: return "Already Reported";
    case DFK_HTTP_IM_USED: return "IM Used";
    case DFK_HTTP_MULTIPLE_CHOICES: return "Multiple Choices";
    case DFK_HTTP_MOVED_PERMANENTLY: return "Moved Permanently";
    case DFK_HTTP_FOUND: return "Found";
    case DFK_HTTP_SEE_OTHER: return "See Other";
    case DFK_HTTP_NOT_MODIFIED: return "Not Modified";
    case DFK_HTTP_USE_PROXY: return "Use Proxy";
    case DFK_HTTP_SWITCH_PROXY: return "Switch Proxy";
    case DFK_HTTP_TEMPORARY_REDIRECT: return "Temporary Redirect";
    case DFK_HTTP_PERMANENT_REDIRECT: return "Permanent Redirect";
    case DFK_HTTP_BAD_REQUEST: return "Bad Request";
    case DFK_HTTP_UNAUTHORIZED: return "Unauthorized";
    case DFK_HTTP_PAYMENT_REQUIRED: return "Payment Required";
    case DFK_HTTP_FORBIDDEN: return "Forbidden";
    case DFK_HTTP_NOT_FOUND: return "Not Found";
    case DFK_HTTP_METHOD_NOT_ALLOWED: return "Method Not Allowed";
    case DFK_HTTP_NOT_ACCEPTABLE: return "Not Acceptable";
    case DFK_HTTP_PROXY_AUTHENTICATION_REQUIRED: return "Proxy Authentication Required";
    case DFK_HTTP_REQUEST_TIMEOUT: return "Request Timeout";
    case DFK_HTTP_CONFLICT: return "Conflict";
    case DFK_HTTP_GONE: return "Gone";
    case DFK_HTTP_LENGTH_REQUIRED: return "Length Required";
    case DFK_HTTP_PRECONDITION_FAILED: return "Precondition Failed";
    case DFK_HTTP_PAYLOAD_TOO_LARGE: return "Payload Too Large";
    case DFK_HTTP_URI_TOO_LONG: return "Uri Too Long";
    case DFK_HTTP_UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
    case DFK_HTTP_RANGE_NOT_SATISFIABLE: return "Range Not Satisfiable";
    case DFK_HTTP_EXPECTATION_FAILED: return "Expectation Failed";
    case DFK_HTTP_I_AM_A_TEAPOT: return "I'm a teapot";
    case DFK_HTTP_MISDIRECTED_REQUEST: return "Misdirected Request";
    case DFK_HTTP_UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
    case DFK_HTTP_LOCKED: return "Locked";
    case DFK_HTTP_FAILED_DEPENDENCY: return "Failed Dependency";
    case DFK_HTTP_UPGRADE_REQUIRED: return "Upgrade Required";
    case DFK_HTTP_PRECONDITION_REQUIRED: return "Precondition Required";
    case DFK_HTTP_TOO_MANY_REQUESTS: return "Too Many Requests";
    case DFK_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE: return "Request Header Fields Too Large";
    case DFK_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS: return "Unavailable For Legal Reasons";
    case DFK_HTTP_INTERNAL_SERVER_ERROR: return "Internal Server Error";
    case DFK_HTTP_NOT_IMPLEMENTED: return "Not Implemented";
    case DFK_HTTP_BAD_GATEWAY: return "Bad Gateway";
    case DFK_HTTP_SERVICE_UNAVAILABLE: return "Service Unavailable";
    case DFK_HTTP_GATEWAY_TIMEOUT: return "Gateway Timeout";
    case DFK_HTTP_HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
    case DFK_HTTP_VARIANT_ALSO_NEGOTIATES: return "Variant Also Negotiates";
    case DFK_HTTP_INSUFFICIENT_STORAGE: return "Insufficient Storage";
    case DFK_HTTP_LOOP_DETECTED: return "Loop Detected";
    case DFK_HTTP_NOT_EXTENDED: return "Not Extended";
    case DFK_HTTP_NETWORK_AUTHENTICATION_REQUIRED: return "Network Authentication Required";
    default: return "Unknown";
  }
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
  DFK_DBG(p->dfk, "{%p} %.*s", (void*) p->req, (int) size, at);
  dfk__buf_append(&p->req->url, at, size);
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
  DFK_DBG(p->dfk, "{%p} %.*s", (void*) p->req, (int) size, at);
  if (!p->cheader || p->cheader->value.data) {
    if (p->cheader) {
      dfk_avltree_insert(&p->req->_.headers, (dfk_avltree_hook_t*) p->cheader);
    }
    p->cheader = dfk_arena_alloc(p->req->_.arena, sizeof(dfk__http_header_t));
    dfk__http_header_init(p->cheader);
  }
  dfk__buf_append(&p->cheader->name, at, size);
  return 0;
}


static int dfk__http_on_header_value(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p} %.*s", (void*) p->req, (int) size, at);
  dfk__buf_append(&p->cheader->value, at, size);
  return 0;
}


static int dfk__http_on_headers_complete(http_parser* parser)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
  if (p->cheader) {
    dfk_avltree_insert(&p->req->_.headers, (dfk_avltree_hook_t*) p->cheader);
    p->cheader = NULL;
  }
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


typedef struct dfk__mutex_list_t {
  dfk_list_hook_t hook;
  dfk_mutex_t mutex;
} dfk__mutex_list_t;


static void dfk__http(dfk_coro_t* coro, dfk_tcp_socket_t* sock, void* p)
{
  dfk_http_t* http = (dfk_http_t*) p;
  http_parser parser;
  http_parser_settings parser_settings;
  dfk_http_req_t req;
  dfk_http_resp_t resp;
  dfk_arena_t arena;
  char buf[DFK_HTTP_HEADERS_BUFFER] = {0};
  dfk__http_parser_data_t pdata;
  dfk__mutex_list_t ml;

  dfk_list_hook_init(&ml.hook);
  DFK_CALL_RVOID(http->dfk, dfk_mutex_init(&ml.mutex, http->dfk));
  DFK_CALL_RVOID(http->dfk, dfk_mutex_lock(&ml.mutex));
  dfk_list_append(&http->_.connections, &ml.hook);

  assert(http);
  dfk_arena_init(&arena, http->dfk);
  dfk__http_req_init(&req, &arena, sock);
  dfk__http_resp_init(&resp, &arena, sock);
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
    DFK_DBG(http->dfk, "{%p} parser step done", (void*) http);
  }

  req.version.major = parser.http_major;
  req.version.minor = parser.http_minor;
  req.method = parser.method;

  DFK_DBG(http->dfk, "{%p} request parsing done, "
      "%s %.*s HTTP/%hu.%hu",
      (void*) http,
      http_method_str((enum http_method) req.method),
      (int) req.url.size, req.url.data,
      req.version.major,
      req.version.minor);

  assert(pdata.done);
  {
    char respbuf[128] = {0};
    int hres = http->_.handler(http, &req, &resp);
    dfk_http_status_e status = ((hres == dfk_err_ok)
      ? resp.code
      : DFK_HTTP_INTERNAL_SERVER_ERROR);
    int size  = snprintf(respbuf, sizeof(respbuf), "HTTP/1.0 %3d %s\r\n",
        status, dfk__http_reason_phrase(status));
    dfk_tcp_socket_write(sock, respbuf, size);
  }

connection_broken:
  dfk__http_parser_data_free(&pdata);
  dfk__http_resp_free(&resp);
  dfk__http_req_free(&req);
  dfk_arena_free(&arena);

  DFK_CALL_RVOID(http->dfk, dfk_tcp_socket_close(sock));
  dfk_list_erase(&http->_.connections, &ml.hook);
  DFK_CALL_RVOID(http->dfk, dfk_mutex_unlock(&ml.mutex));
}

dfk_buf_t dfk_http_get(dfk_http_req_t* req, const char* name, size_t namesize)
{
  if (!req || (!name && namesize)) {
    return (dfk_buf_t) {NULL, 0};
  }
  {
    dfk_buf_t e = (dfk_buf_t) {(char*) name, namesize};
    dfk__http_header_t* h = (dfk__http_header_t*)
      dfk_avltree_lookup(&req->_.headers, &e, dfk__http_header_lookup_cmp);
    if (h) {
      return h->value;
    } else {
      return (dfk_buf_t) {NULL, 0};
    }
  }
}


int dfk_http_headers_begin(dfk_http_req_t* req, dfk_http_headers_it* it)
{
  if (!req || !it) {
    return dfk_err_badarg;
  }
  dfk_avltree_it_init(&req->_.headers, &it->_.it);
  it->value.field = ((dfk__http_header_t*) it->_.it.value)->name;
  it->value.value = ((dfk__http_header_t*) it->_.it.value)->value;
  return dfk_err_ok;
}


int dfk_http_headers_next(dfk_http_headers_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  if (!dfk_avltree_it_valid(&it->_.it)) {
    return dfk_err_eof;
  }
  dfk_avltree_it_next(&it->_.it);
  if (dfk_avltree_it_valid(&it->_.it)) {
    it->value.field = ((dfk__http_header_t*) it->_.it.value)->name;
    it->value.value = ((dfk__http_header_t*) it->_.it.value)->value;
  }
  return dfk_err_ok;
}


int dfk_http_headers_valid(dfk_http_headers_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  return dfk_avltree_it_valid(&it->_.it) ? dfk_err_ok : dfk_err_eof;
}


int dfk_http_init(dfk_http_t* http, dfk_t* dfk)
{
  if (!http || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) http);
  http->dfk = dfk;
  dfk_list_init(&http->_.connections);
  return dfk_tcp_socket_init(&http->_.listensock, dfk);
}


int dfk_http_stop(dfk_http_t* http)
{
  if (!http) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  DFK_CALL(http->dfk, dfk_tcp_socket_close(&http->_.listensock));
  {
    dfk_list_hook_t* i = http->_.connections.head;
    while (i) {
      dfk__mutex_list_t* it = (dfk__mutex_list_t*) i;
      DFK_DBG(http->dfk, "{%p} waiting for {%p} to terminate", (void*) http, (void*) i);
      DFK_CALL(http->dfk, dfk_mutex_lock(&it->mutex));
      DFK_CALL(http->dfk, dfk_mutex_unlock(&it->mutex));
      DFK_CALL(http->dfk, dfk_mutex_free(&it->mutex));
      i = http->_.connections.head;
    }
  }
  return dfk_err_ok;
}


int dfk_http_free(dfk_http_t* http)
{
  if (!http) {
    return dfk_err_badarg;
  }
  dfk_list_free(&http->_.connections);
  return dfk_tcp_socket_free(&http->_.listensock);
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
  dfk_list_append(&http->dfk->_.http_servers, (dfk_list_hook_t*) http);
  return dfk_tcp_socket_listen(&http->_.listensock, endpoint, port, dfk__http, http, 0);
}

