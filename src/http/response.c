/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <dfk/error.h>
#include <dfk/http/response.h>
#include <dfk/internal/http/response.h>
#include <dfk/http/server.h>
#include <dfk/internal.h>

static ssize_t dfk__mocked_write(dfk_http_response_t* resp,
    char* buf, size_t nbytes)
{
#if DFK_MOCKS
    if (resp->_socket_mocked) {
      return dfk__sponge_write(resp->_socket_mock, buf, nbytes);
    } else {
      return dfk_tcp_socket_write(resp->_socket, buf, nbytes);
    }
#else
    return dfk_tcp_socket_write(resp->_socket, buf, nbytes);
#endif
}

static ssize_t dfk__mocked_writev(dfk_http_response_t* resp,
    dfk_iovec_t* iov, size_t niov)
{
#if DFK_MOCKS
    if (resp->_socket_mocked) {
      return dfk__sponge_writev(resp->_socket_mock, iov, niov);
    } else {
      return dfk_tcp_socket_writev(resp->_socket, iov, niov);
    }
#else
    return dfk_tcp_socket_writev(resp->_socket, iov, niov);
#endif
}


void dfk__http_response_init(dfk_http_response_t* resp, dfk_http_request_t* req,
    dfk_arena_t* request_arena, dfk_arena_t* connection_arena,
    dfk_tcp_socket_t* sock, int keepalive)
{
  assert(resp);
  assert(req);
  assert(request_arena);
  assert(connection_arena);
  assert(sock);

  resp->_request_arena = request_arena;
  resp->_connection_arena = connection_arena;
  resp->_socket = sock;
  dfk_strmap_init(&resp->headers);
#if DFK_MOCKS
  resp->_socket_mocked = 0;
  resp->_socket_mock = 0;
#endif
  resp->_headers_flushed = 0;

  resp->http = req->http;
  resp->major_version = req->major_version;
  resp->minor_version = req->minor_version;
  resp->status = DFK_HTTP_OK;
  resp->content_length = (size_t) -1;
  resp->chunked = 0;
  resp->keepalive = !!keepalive;
}

size_t dfk_http_response_sizeof(void)
{
  return sizeof(dfk_http_response_t);
}

int dfk__http_response_flush_headers(dfk_http_response_t* resp)
{
  assert(resp);
  if (resp->_headers_flushed) {
    return dfk_err_ok;
  }

  /* Set "Content-Length" header if not specified manually */
  if (resp->content_length != (size_t) -1) {
    dfk_buf_t content_length = dfk_strmap_get(&resp->headers, DFK_HTTP_CONTENT_LENGTH, sizeof(DFK_HTTP_CONTENT_LENGTH) - 1);
    if (!content_length.data) {
      char buf[64] = {0};
      size_t len = snprintf(buf, sizeof(buf), "%llu", (unsigned long long) resp->content_length);
      dfk_strmap_item_t* cl = dfk_strmap_item_acopy_value(resp->_request_arena, DFK_HTTP_CONTENT_LENGTH, sizeof(DFK_HTTP_CONTENT_LENGTH) - 1, buf, len);
      dfk_strmap_insert(&resp->headers, cl);
    }
  }

  size_t totalheaders = dfk_strmap_size(&resp->headers);
  size_t niov = 4 * totalheaders + 2;
  dfk_iovec_t* iov = dfk_arena_alloc(resp->_request_arena, niov * sizeof(dfk_iovec_t));
  if (!iov) {
    resp->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
  }

  char sbuf[128] = {0};
  int ssize = snprintf(sbuf, sizeof(sbuf), "HTTP/%d.%d %3d %s\r\n",
                       resp->major_version, resp->minor_version, resp->status,
                       dfk_http_reason_phrase(resp->status));
  if (!iov) {
    /** @todo return value */
    dfk__mocked_write(resp, sbuf, ssize);
  } else {
    size_t i = 0;
    iov[i++] = (dfk_iovec_t) {sbuf, ssize};
    dfk_strmap_it it;
    dfk_strmap_it end;
    dfk_strmap_begin(&resp->headers, &it);
    dfk_strmap_end(&resp->headers, &end);
    while (!dfk_strmap_it_equal(&it, &end) && i < niov) {
      iov[i++] = (dfk_iovec_t) {(char*) it.item->key.data, it.item->key.size};
      iov[i++] = (dfk_iovec_t) {": ", 2};
      iov[i++] = (dfk_iovec_t) {it.item->value.data, it.item->value.size};
      iov[i++] = (dfk_iovec_t) {"\r\n", 2};
      dfk_strmap_it_next(&it);
    }
    iov[i++] = (dfk_iovec_t) {"\r\n", 2};
    /** @todo return code */
    dfk__mocked_writev(resp, iov, niov);
  }
  resp->_headers_flushed |= 1;
  return dfk_err_ok;
}

ssize_t dfk_http_response_write(dfk_http_response_t* resp, char* buf, size_t nbytes)
{
  assert(resp);
  assert(buf && nbytes);
  dfk_iovec_t iov = {buf, nbytes};
  return dfk_http_response_writev(resp, &iov, 1);
}

ssize_t dfk_http_response_writev(dfk_http_response_t* resp, dfk_iovec_t* iov, size_t niov)
{
  assert(resp);
  assert(iov && niov);
  dfk__http_response_flush_headers(resp);
  return dfk__mocked_writev(resp, iov, niov);
}

int dfk_http_response_set(dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen)
{
  assert(resp);
  assert(name && namelen);
  assert(value && valuelen);
  DFK_DBG(resp->http->dfk, "{%p} '%.*s': '%.*s'", (void*) resp,
      (int) namelen, name, (int) valuelen, value);
  dfk_strmap_item_t* item = dfk_arena_alloc(
      resp->_request_arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return dfk_err_nomem;
  }
  dfk_strmap_item_init(item, name, namelen, (char*) value, valuelen);
  dfk_strmap_insert(&resp->headers, item);
  return dfk_err_ok;
}

int dfk_http_response_set_copy(
    dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen)
{
  assert(resp);
  assert(name && namelen);
  assert(value && valuelen);
  DFK_DBG(resp->http->dfk, "{%p} '%.*s': '%.*s'", (void*) resp,
      (int) namelen, name, (int) valuelen, value);
  dfk_strmap_item_t* i = dfk_strmap_item_acopy(
      resp->_request_arena, name, namelen, value, valuelen);
  if (!i) {
    return dfk_err_nomem;
  }
  dfk_strmap_insert(&resp->headers, i);
  return dfk_err_ok;
}

int dfk_http_response_set_copy_name(
    dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen)
{
  assert(resp);
  assert(name && namelen);
  assert(value && valuelen);
  DFK_DBG(resp->http->dfk, "{%p} '%.*s': '%.*s'", (void*) resp,
      (int) namelen, name, (int) valuelen, value);
  dfk_strmap_item_t* i = dfk_strmap_item_acopy_key(
      resp->_request_arena, name, namelen, (char*) value, valuelen);
  if (!i) {
    return dfk_err_nomem;
  }
  dfk_strmap_insert(&resp->headers, i);
  return dfk_err_ok;
}

int dfk_http_response_set_copy_value(
    dfk_http_response_t* resp,
    const char* name, size_t namelen, const char* value, size_t valuelen)
{
  assert(resp);
  assert(name && namelen);
  assert(value && valuelen);
  DFK_DBG(resp->http->dfk, "{%p} '%.*s': '%.*s'", (void*) resp,
      (int) namelen, name, (int) valuelen, value);
  dfk_strmap_item_t* i = dfk_strmap_item_acopy_value(
      resp->_request_arena, name, namelen, value, valuelen);
  if (!i) {
    return dfk_err_nomem;
  }
  dfk_strmap_insert(&resp->headers, i);
  return dfk_err_ok;
}

int dfk_http_response_bset(dfk_http_response_t* resp,
                           dfk_buf_t name, dfk_buf_t value)
{
  return dfk_http_response_set(
      resp, name.data, name.size, value.data, value.size);
}

int dfk_http_response_bset_copy(dfk_http_response_t* resp,
                                dfk_buf_t name, dfk_buf_t value)
{
  return dfk_http_response_set_copy(
      resp, name.data, name.size, value.data, value.size);
}

int dfk_http_response_bset_copy_name(dfk_http_response_t* resp,
                                     dfk_buf_t name, dfk_buf_t value)
{
  return dfk_http_response_set_copy_name(
      resp, name.data, name.size, value.data, value.size);
}

int dfk_http_response_bset_copy_value(dfk_http_response_t* resp,
                                      dfk_buf_t name, dfk_buf_t value)
{
  return dfk_http_response_set_copy_value(
      resp, name.data, name.size, value.data, value.size);
}

