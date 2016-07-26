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
#include <dfk/http/response.h>
#include <dfk/internal.h>


void dfk__http_response_init(dfk_http_response_t* resp, dfk_t* dfk,
                             dfk_arena_t* request_arena,
                             dfk_arena_t* connection_arena,
                             dfk_tcp_socket_t* sock)
{
  assert(resp);
  assert(request_arena);
  assert(connection_arena);
  assert(sock);

  resp->_request_arena = request_arena;
  resp->_connection_arena = connection_arena;
  resp->_sock = sock;
  dfk_avltree_init(&resp->_headers, dfk__http_headers_cmp);
#if DFK_MOCKS
  resp->_sock_mocked = 0;
  resp->_sock_mock = 0;
#endif
  resp->_headers_flushed = 0;

  resp->dfk = dfk;
  resp->major_version = 1;
  resp->minor_version = 0;
  resp->code = DFK_HTTP_OK;
  resp->content_length = (size_t) -1;
  resp->chunked = 0;
}


void dfk__http_response_free(dfk_http_response_t* resp)
{
  assert(resp);
  dfk_avltree_free(&resp->_headers);
}


int dfk_http_set(dfk_http_response_t* resp, const char* name, size_t namesize, const char* value, size_t valuesize)
{
  if (!resp || (!name && namesize) || (!value && valuesize)) {
    return dfk_err_badarg;
  }
  DFK_DBG(resp->dfk, "{%p} %.*s: %.*s", (void*) resp, (int) namesize, name,
          (int) valuesize, value);
  dfk_http_header_t* header = dfk_arena_alloc(resp->_request_arena,
                                              sizeof(dfk_http_header_t));
  if (!header) {
    return dfk_err_nomem;
  }
  dfk__http_header_init(header);
  header->name = (dfk_buf_t) {(char*) name, namesize};
  header->value = (dfk_buf_t) {(char*) value, valuesize};
  dfk_avltree_insert(&resp->_headers, (dfk_avltree_hook_t*) header);
  return dfk_err_ok;
}


int dfk_http_set_copy(dfk_http_response_t* resp, const char* name,
                      size_t namesize, const char* value, size_t valuesize)
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


int dfk_http_set_copy_name(dfk_http_response_t* resp, const char* name,
                           size_t namesize, const char* value,
                           size_t valuesize)
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


int dfk_http_set_copy_value(dfk_http_response_t* resp, const char* name,
                            size_t namesize, const char* value,
                            size_t valuesize)
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


int dfk__http_response_flush_headers(dfk_http_response_t* resp)
{
  assert(resp);
  if (resp->_headers_flushed) {
    return dfk_err_ok;
  }

  /* Set "Content-Length" header if not specified manually */
  if (resp->content_length != (size_t) -1) {
    dfk_buf_t content_length = dfk_http_response_get(resp, DFK_HTTP_CONTENT_LENGTH, sizeof(DFK_HTTP_CONTENT_LENGTH) - 1);
    if (!content_length.data) {
      char buf[64] = {0};
      size_t len = snprintf(buf, sizeof(buf), "%llu", (unsigned long long) resp->content_length);
      dfk_http_set_copy_value(resp, DFK_HTTP_CONTENT_LENGTH, sizeof(DFK_HTTP_CONTENT_LENGTH) - 1, buf, len);
    }
  }

  size_t totalheaders = dfk_avltree_size(&resp->_headers);
  size_t niov = 4 * totalheaders + 2;
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
      dfk_http_header_t* h = (dfk_http_header_t*) it.value;
      iov[i++] = h->name;
      iov[i++] = (dfk_iovec_t) {": ", 2};
      iov[i++] = h->value;
      iov[i++] = (dfk_iovec_t) {"\r\n", 2};
      dfk_avltree_it_next(&it);
    }
    iov[i++] = (dfk_iovec_t) {"\r\n", 2};
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
  resp->_headers_flushed |= 1;
  return dfk_err_ok;
}


int dfk__http_response_flush(dfk_http_response_t* resp)
{
  assert(resp);
  return dfk__http_response_flush_headers(resp);
}


dfk_buf_t dfk_http_response_get(dfk_http_response_t* resp,
                                const char* name, size_t namesize)
{
  if (!resp) {
    return (dfk_buf_t) {NULL, 0};
  }
  if (!name && namesize) {
    resp->dfk->dfk_errno = dfk_err_badarg;
    return (dfk_buf_t) {NULL, 0};
  }
  return dfk__http_headers_get(&resp->_headers, name, namesize);
}


int dfk_http_response_headers_begin(dfk_http_response_t* resp,
                                    dfk_http_header_it* it)
{
  if (!resp || !it) {
    return dfk_err_badarg;
  }
  return dfk__http_headers_begin(&resp->_headers, it);
}


ssize_t dfk_http_write(dfk_http_response_t* resp, char* buf, size_t nbytes)
{
  if (!resp) {
    return -1;
  }
  if (!buf && nbytes) {
    resp->dfk->dfk_errno = dfk_err_badarg;
    return -1;
  }
  dfk_iovec_t iov = {buf, nbytes};
  return dfk_http_writev(resp, &iov, 1);
}


ssize_t dfk_http_writev(dfk_http_response_t* resp, dfk_iovec_t* iov, size_t niov)
{
  if (!resp) {
    return -1;
  }
  if (!iov && niov) {
    resp->dfk->dfk_errno = dfk_err_badarg;
    return -1;
  }
  dfk__http_response_flush_headers(resp);
#if DFK_MOCKS
  if (resp->_sock_mocked) {
    return dfk_sponge_writev(resp->_sock_mock, iov, niov);
  } else {
    return dfk_tcp_socket_writev(resp->_sock, iov, niov);
  }
#else
  return dfk_tcp_socket_writev(resp->_sock, iov, niov);
#endif
}

