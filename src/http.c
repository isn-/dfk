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

#include <assert.h>
#include <http_parser.h>
#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/misc.h>
#include <dfk/internal/http.h>


#define DFK_HTTP_USER_AGENT "User-Agent"
#define DFK_HTTP_HOST "Host"
#define DFK_HTTP_ACCEPT "Accept"
#define DFK_HTTP_CONTENT_TYPE "Content-Type"
#define DFK_HTTP_CONTENT_LENGTH "Content-Length"


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


void dfk__http_req_init(dfk_http_req_t* req, dfk_t* dfk, dfk_arena_t* request_arena, dfk_arena_t* connection_arena, dfk_tcp_socket_t* sock)
{
  assert(req);
  assert(request_arena);
  assert(connection_arena);
  assert(sock);
  req->_request_arena = request_arena;
  req->_connection_arena = connection_arena;
  req->_sock = sock;
  dfk_avltree_init(&req->_headers, dfk__http_header_cmp);
  dfk_avltree_init(&req->_arguments, dfk__http_header_cmp);
  req->_bodypart = (dfk_buf_t) {NULL, 0};
  req->_body_bytes_nread = 0;
  req->_headers_done = 0;
  req->dfk = dfk;
  req->url = (dfk_buf_t) {NULL, 0};
  req->user_agent = (dfk_buf_t) {NULL, 0};
  req->host = (dfk_buf_t) {NULL, 0};
  req->content_type = (dfk_buf_t) {NULL, 0};
  req->content_length = 0;
}


void dfk__http_req_free(dfk_http_req_t* req)
{
  dfk_avltree_free(&req->_arguments);
  dfk_avltree_free(&req->_headers);
}


void dfk__http_resp_init(dfk_http_resp_t* resp, dfk_t* dfk, dfk_arena_t* request_arena, dfk_arena_t* connection_arena, dfk_tcp_socket_t* sock)
{
  assert(resp);
  assert(request_arena);
  assert(connection_arena);
  assert(sock);
  resp->_request_arena = request_arena;
  resp->_connection_arena = connection_arena;
  resp->_sock = sock;
  dfk_avltree_init(&resp->_headers, dfk__http_header_cmp);
  resp->dfk = dfk;
#if DFK_MOCKS
  resp->_sock_mocked = 0;
#endif
}


void dfk__http_resp_free(dfk_http_resp_t* resp)
{
  assert(resp);
  DFK_UNUSED(resp);
}


typedef struct dfk__http_parser_data_t {
  dfk_t* dfk;
  dfk_http_req_t* req;
  dfk__http_header_t* cheader; /* current header */
  int keepalive;
} dfk__http_parser_data_t;


static void dfk__http_parser_data_init(dfk__http_parser_data_t* pdata, dfk_t* dfk)
{
  pdata->dfk = dfk;
  pdata->req = NULL;
  pdata->cheader = NULL;
  pdata->keepalive = 1;
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


const char* dfk__http_reason_phrase(dfk_http_status_e status)
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


static int dfk__http_on_header_field(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p} %.*s", (void*) p->req, (int) size, at);
  if (!p->cheader || p->cheader->value.data) {
    if (p->cheader) {
      dfk_avltree_insert(&p->req->_headers, (dfk_avltree_hook_t*) p->cheader);
    }
    p->cheader = dfk_arena_alloc(p->req->_request_arena, sizeof(dfk__http_header_t));
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
    dfk_avltree_insert(&p->req->_headers, (dfk_avltree_hook_t*) p->cheader);
    p->cheader = NULL;
  }
  p->req->_headers_done = 1;
  p->keepalive = http_should_keep_alive(parser);
  return 0;
}


static int dfk__http_on_body(http_parser* parser, const char* at, size_t size)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p} %llu bytes", (void*) p->req, (unsigned long long) size);
  dfk__buf_append(&p->req->_bodypart, at, size);
  return 0;
}


static int dfk__http_on_message_complete(http_parser* parser)
{
  dfk__http_parser_data_t* p = (dfk__http_parser_data_t*) parser->data;
  DFK_DBG(p->dfk, "{%p}", (void*) p->req);
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


typedef struct dfk__mutex_list_item_t {
  dfk_list_hook_t hook;
  dfk_mutex_t mutex;
} dfk__mutex_list_item_t;


static void dfk__http(dfk_coro_t* coro, dfk_tcp_socket_t* sock, void* p)
{
  dfk_http_t* http = (dfk_http_t*) p;
  char buf[DFK_HTTP_HEADERS_BUFFER] = {0};

  /*
   * dfk_list_hook_t + dfk_mutex_t to be placed into dfk_http_t._connections
   * list. When dfk_http_stop is called, it waits for all current requests
   * to be processed gracefully by locking each mutex. The mutex is inserted
   * into the dfk_http_t._connections list in a locked state and is released
   * only when connection is closed indicating that no wait is required for
   * this connection.
   */
  dfk__mutex_list_item_t ml;
  dfk_list_hook_init(&ml.hook);
  DFK_CALL_RVOID(http->dfk, dfk_mutex_init(&ml.mutex, http->dfk));
  DFK_CALL_RVOID(http->dfk, dfk_mutex_lock(&ml.mutex));
  dfk_list_append(&http->_connections, &ml.hook);

  assert(http);

  /* Arena for per-connection data */
  dfk_arena_t connection_arena;
  dfk_arena_init(&connection_arena, http->dfk);

  /* A pointer to pdata is passed to http_parser callbacks */
  dfk__http_parser_data_t pdata;
  dfk__http_parser_data_init(&pdata, coro->dfk);

  /* Initialize http_parser instance */
  http_parser parser;
  http_parser_settings parser_settings;
  http_parser_init(&parser, HTTP_REQUEST);
  parser.data = &pdata;
  http_parser_settings_init(&parser_settings);
  parser_settings.on_message_begin = dfk__http_on_message_begin;
  parser_settings.on_url = dfk__http_on_url;
  parser_settings.on_status = NULL; /* on_status is response-only */
  parser_settings.on_header_field = dfk__http_on_header_field;
  parser_settings.on_header_value = dfk__http_on_header_value;
  parser_settings.on_headers_complete = dfk__http_on_headers_complete;
  parser_settings.on_body = dfk__http_on_body;
  parser_settings.on_message_complete = dfk__http_on_message_complete;
  parser_settings.on_chunk_header = dfk__http_on_chunk_header;
  parser_settings.on_chunk_complete = dfk__http_on_chunk_complete;

  /* Requests processed within this connection */
  size_t nrequests = 0;

  while (pdata.keepalive) {
    dfk_arena_t request_arena;
    dfk_arena_init(&request_arena, http->dfk);

    dfk_http_req_t req;
    dfk__http_req_init(&req, http->dfk, &request_arena, &connection_arena, sock);

    dfk_http_resp_t resp;
    dfk__http_resp_init(&resp, http->dfk, &request_arena, &connection_arena, sock);

    pdata.req = &req;

    /* Read headers first */
    while (!req._headers_done) {
      ssize_t nread = dfk_tcp_socket_read(sock, buf, sizeof(buf));
      if (nread < 0) {
        pdata.keepalive = 0;
        goto connection_broken;
      }
      DFK_DBG(http->dfk, "{%p} execute parser", (void*) http);
      http_parser_execute(&parser, &parser_settings, buf, nread);
      DFK_DBG(http->dfk, "{%p} parser step done", (void*) http);
    }

    /* Request pre-processing */
    DFK_DBG(http->dfk, "{%p} populate common headers", (void*) http);
    req.major_version = parser.http_major;
    req.minor_version = parser.http_minor;
    req.method = parser.method;
    req.user_agent = dfk_http_get(&req, DFK_HTTP_USER_AGENT, sizeof(DFK_HTTP_USER_AGENT) - 1);
    req.host = dfk_http_get(&req, DFK_HTTP_HOST, sizeof(DFK_HTTP_HOST) - 1);
    req.accept = dfk_http_get(&req, DFK_HTTP_ACCEPT, sizeof(DFK_HTTP_ACCEPT) - 1);
    req.content_type = dfk_http_get(&req, DFK_HTTP_CONTENT_TYPE, sizeof(DFK_HTTP_CONTENT_TYPE) - 1);
    {
      /* Parse and set content_length member */
      dfk_buf_t content_length = dfk_http_get(&req, DFK_HTTP_CONTENT_LENGTH, sizeof(DFK_HTTP_CONTENT_LENGTH) - 1);
      if (content_length.size) {
        long long intval;
        int res = dfk_strtoll(content_length, NULL, 10, &intval);
        if (res != dfk_err_ok) {
          DFK_WARNING(http->dfk, "{%p} malformed value for \"" DFK_HTTP_CONTENT_LENGTH "\" header: %.*s",
              (void*) http, (int) content_length.size, content_length.data);
        } else {
          req.content_length = (size_t) intval;
        }
      }
    }

    DFK_DBG(http->dfk, "{%p} request parsing done, "
        "%s %.*s HTTP/%hu.%hu \"%.*s\"",
        (void*) http,
        http_method_str((enum http_method) req.method),
        (int) req.url.size, req.url.data,
        req.major_version,
        req.minor_version,
        (int) req.user_agent.size, req.user_agent.data);

    resp.major_version = 1;
    resp.minor_version = 0;

    int hres = http->_handler(http, &req, &resp);
    DFK_INFO(http->dfk, "{%p} http handler returned %d", (void*) http, hres);
    if (hres != dfk_err_ok) {
      resp.code = DFK_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (nrequests + 1 >= http->keepalive_requests) {
      DFK_INFO(http->dfk, "{%p} maximum number of keepalive requests (%llu) "
               "for connection {%p} has reached, close connection",
               (void*) http, (unsigned long long) http->keepalive_requests,
               (void*) sock);
      pdata.keepalive = 0;
    }

    /*
     * @todo Check for Connection: close response header here,
     * if presented, set pdata.keepalive = 0
     */
    pdata.keepalive = 0;

    dfk__http_resp_flush(&resp);

    ++nrequests;

connection_broken:
    dfk__http_resp_free(&resp);
    dfk__http_req_free(&req);
    dfk_arena_free(&request_arena);
  }

  dfk__http_parser_data_free(&pdata);
  dfk_arena_free(&connection_arena);

  DFK_CALL_RVOID(http->dfk, dfk_tcp_socket_close(sock));
  dfk_list_erase(&http->_connections, &ml.hook);
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
      dfk_avltree_lookup(&req->_headers, &e, dfk__http_header_lookup_cmp);
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
  dfk_avltree_it_init(&req->_headers, &it->_it);
  it->field = ((dfk__http_header_t*) it->_it.value)->name;
  it->value = ((dfk__http_header_t*) it->_it.value)->value;
  return dfk_err_ok;
}


int dfk_http_headers_next(dfk_http_headers_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  if (!dfk_avltree_it_valid(&it->_it)) {
    return dfk_err_eof;
  }
  dfk_avltree_it_next(&it->_it);
  if (dfk_avltree_it_valid(&it->_it)) {
    it->field = ((dfk__http_header_t*) it->_it.value)->name;
    it->value = ((dfk__http_header_t*) it->_it.value)->value;
  }
  return dfk_err_ok;
}


int dfk_http_headers_valid(dfk_http_headers_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  return dfk_avltree_it_valid(&it->_it) ? dfk_err_ok : dfk_err_eof;
}


ssize_t dfk_http_read(dfk_http_req_t* req, char* buf, size_t size)
{
  if (!req || (!buf && size)) {
    return dfk_err_badarg;
  }

  DFK_DBG(req->dfk, "{%p} upto %llu bytes", (void*) req, (unsigned long long) size);

  if (!req->content_length) {
    return dfk_err_eof;
  }

  if (req->_body_bytes_nread < req->_bodypart.size) {
    DFK_DBG(req->dfk, "{%p} body part of size %llu is cached, copy",
            (void*) req, (unsigned long long) req->_bodypart.size);
    size_t tocopy = DFK_MIN(size, req->_bodypart.size - req->_body_bytes_nread);
    memcpy(buf, req->_bodypart.data, tocopy);
    req->_body_bytes_nread += tocopy;
    return tocopy;
  }
  size_t toread = DFK_MIN(size, req->content_length - req->_body_bytes_nread);
  return dfk_tcp_socket_read(req->_sock, buf, toread);
}


ssize_t dfk_http_readv(dfk_http_req_t* req, dfk_iovec_t* iov, size_t niov)
{
  if (!req || (!iov && niov)) {
    return dfk_err_badarg;
  }
  DFK_DBG(req->dfk, "{%p} into %llu blocks", (void*) req, (unsigned long long) niov);
  return dfk_http_read(req, iov[0].data, iov[0].size);
}


int dfk_http_set(dfk_http_resp_t* resp, const char* name, size_t namesize, const char* value, size_t valuesize)
{
  if (!resp || (!name && namesize) || (!value && valuesize)) {
    return dfk_err_badarg;
  }
  DFK_DBG(resp->dfk, "{%p} %.*s: %.*s", (void*) resp, (int) namesize, name, (int) valuesize, value);
  dfk__http_header_t* header = dfk_arena_alloc(resp->_request_arena, sizeof(dfk__http_header_t));
  if (!header) {
    return dfk_err_nomem;
  }
  dfk__http_header_init(header);
  header->name = (dfk_buf_t) {(char*) name, namesize};
  header->value = (dfk_buf_t) {(char*) value, valuesize};
  dfk_avltree_insert(&resp->_headers, (dfk_avltree_hook_t*) header);
  return dfk_err_ok;
}


int dfk_http_set_copy(dfk_http_resp_t* resp, const char* name, size_t namesize, const char* value, size_t valuesize)
{
  if (!resp || (!name && namesize) || (!value && valuesize)) {
    return dfk_err_badarg;
  }
  DFK_DBG(resp->dfk, "{%p} %.*s: %.*s", (void*) resp, (int) namesize, name, (int) valuesize, value);
  void* namecopy = dfk_arena_alloc_copy(resp->_request_arena, name, namesize);
  if (!namecopy) {
    return dfk_err_nomem;
  }
  void* valuecopy = dfk_arena_alloc_copy(resp->_request_arena, value, valuesize);
  if (!valuecopy) {
    return dfk_err_nomem;
  }
  return dfk_http_set(resp, namecopy, namesize, valuecopy, valuesize);
}


int dfk_http_set_copy_name(dfk_http_resp_t* resp, const char* name, size_t namesize, const char* value, size_t valuesize)
{
  if (!resp || (!name && namesize) || (!value && valuesize)) {
    return dfk_err_badarg;
  }
  DFK_DBG(resp->dfk, "{%p} %.*s: %.*s", (void*) resp, (int) namesize, name, (int) valuesize, value);
  void* namecopy = dfk_arena_alloc_copy(resp->_request_arena, name, namesize);
  if (!namecopy) {
    return dfk_err_nomem;
  }
  return dfk_http_set(resp, namecopy, namesize, value, valuesize);
}


int dfk_http_set_copy_value(dfk_http_resp_t* resp, const char* name, size_t namesize, const char* value, size_t valuesize)
{
  if (!resp || (!name && namesize) || (!value && valuesize)) {
    return dfk_err_badarg;
  }
  DFK_DBG(resp->dfk, "{%p} %.*s: %.*s", (void*) resp, (int) namesize, name, (int) valuesize, value);
  void* valuecopy = dfk_arena_alloc_copy(resp->_request_arena, value, valuesize);
  if (!valuecopy) {
    return dfk_err_nomem;
  }
  return dfk_http_set(resp, name, namesize, valuecopy, valuesize);
}


int dfk__http_resp_flush_headers(dfk_http_resp_t* resp)
{
  assert(resp);
  size_t totalheaders = dfk_avltree_size(&resp->_headers);
  size_t niov = 4 * totalheaders + 1;
  dfk_iovec_t* iov = dfk_arena_alloc(resp->_request_arena, niov * sizeof(dfk_iovec_t));
  if (!iov) {
    resp->code = DFK_HTTP_INTERNAL_SERVER_ERROR;
  }
  char sbuf[128] = {0};
  int ssize = snprintf(sbuf, sizeof(sbuf), "HTTP/%d.%d %3d %s\r\n",
                       resp->major_version, resp->minor_version, resp->code,
                       dfk__http_reason_phrase(resp->code));
  if (!iov) {
#if DFK_MOCKS
    if (resp->_sock_mocked) {
      dfk_sponge_write(resp->_sock_mock, sbuf, ssize);
    } else {
      dfk_tcp_socket_write(resp->_sock, sbuf, ssize);
    }
#else
    dfk_tcp_socket_write(resp->_sock, sbuf, ssize);
#endif
  } else {
    size_t i = 0;
    iov[i++] = (dfk_iovec_t) {sbuf, ssize};
    dfk_avltree_it_t it;
    dfk_avltree_it_init(&resp->_headers, &it);
    while (dfk_avltree_it_valid(&it) && i < niov) {
      dfk__http_header_t* h = (dfk__http_header_t*) it.value;
      iov[i++] = h->name;
      iov[i++] = (dfk_iovec_t) {": ", 2};
      iov[i++] = h->value;
      iov[i++] = (dfk_iovec_t) {"\r\n", 2};
      dfk_avltree_it_next(&it);
    }
#if DFK_MOCKS
    if (resp->_sock_mocked) {
      dfk_sponge_writev(resp->_sock_mock, iov, niov);
    } else {
      dfk_tcp_socket_writev(resp->_sock, iov, niov);
    }
#else
    dfk_tcp_socket_writev(resp->_sock, iov, niov);
#endif
  }
  return dfk_err_ok;
}


int dfk__http_resp_flush(dfk_http_resp_t* resp)
{
  assert(resp);
  return dfk__http_resp_flush_headers(resp);
}


int dfk_http_init(dfk_http_t* http, dfk_t* dfk)
{
  if (!http || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) http);
  http->dfk = dfk;
  http->keepalive_requests = 100;
  dfk_list_init(&http->_connections);
  return dfk_tcp_socket_init(&http->_listensock, dfk);
}


int dfk_http_stop(dfk_http_t* http)
{
  if (!http) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  DFK_CALL(http->dfk, dfk_tcp_socket_close(&http->_listensock));
  {
    dfk_list_hook_t* i = http->_connections.head;
    while (i) {
      dfk__mutex_list_item_t* it = (dfk__mutex_list_item_t*) i;
      DFK_DBG(http->dfk, "{%p} waiting for {%p} to terminate", (void*) http, (void*) i);
      DFK_CALL(http->dfk, dfk_mutex_lock(&it->mutex));
      DFK_CALL(http->dfk, dfk_mutex_unlock(&it->mutex));
      DFK_CALL(http->dfk, dfk_mutex_free(&it->mutex));
      i = http->_connections.head;
    }
  }
  return dfk_err_ok;
}


int dfk_http_free(dfk_http_t* http)
{
  if (!http) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  dfk_list_free(&http->_connections);
  return dfk_tcp_socket_free(&http->_listensock);
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
  http->_handler = handler;
  dfk_list_append(&http->dfk->_http_servers, (dfk_list_hook_t*) http);
  return dfk_tcp_socket_listen(&http->_listensock, endpoint, port, dfk__http, http, 0);
}

