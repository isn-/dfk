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
#include <dfk/http/server.h>
#include <dfk/http/request.h>
#include <dfk/internal.h>
#include <dfk/internal/misc.h>


static ssize_t dfk_http_request_mocked_read(dfk_http_request_t* req, char* buf, size_t toread)
{
#if DFK_MOCKS
  if (req->_sock_mocked) {
    return dfk_sponge_read(req->_sock_mock, buf, toread);
  } else {
    return dfk_tcp_socket_read(req->_sock, buf, toread);
  }
#else
  return dfk_tcp_socket_read(req->_sock, buf, toread);
#endif
}


static int dfk_http_request_allocate_headers_buf(dfk_http_request_t* req, dfk_buf_t* outbuf)
{
  assert(req);
  assert(outbuf);
  char* buf = DFK_MALLOC(req->http->dfk, req->http->headers_buffer_size);
  if (!buf) {
    return dfk_err_nomem;
  }
  *outbuf = (dfk_buf_t) {buf, req->http->headers_buffer_size};

  dfk_buflist_item_t* buflist_item = dfk_arena_alloc(req->_request_arena,
                                                     sizeof(dfk_buflist_item_t));
  if (!buflist_item) {
    DFK_FREE(req->http->dfk, buf);
    return dfk_err_nomem;
  }
  buflist_item->buf = *outbuf;
  dfk_list_hook_init(&buflist_item->hook);
  dfk_list_append(&req->_buffers, &buflist_item->hook);
  return dfk_err_ok;
}


int dfk_http_request_init(dfk_http_request_t* req, dfk_http_t* http,
                          dfk_arena_t* request_arena,
                          dfk_arena_t* connection_arena,
                          dfk_tcp_socket_t* sock)
{
  assert(req);
  assert(request_arena);
  assert(connection_arena);
  assert(sock);

  memset(req, 0, sizeof(dfk_http_request_t));
  req->_request_arena = request_arena;
  req->_connection_arena = connection_arena;
  req->_sock = sock;
  dfk_list_init(&req->_buffers);
  DFK_CALL(http->dfk, dfk_strmap_init(&req->headers));
  DFK_CALL(http->dfk, dfk_strmap_init(&req->arguments));
  req->http = http;
  req->content_length = -1;
  return dfk_err_ok;
}


int dfk_http_request_free(dfk_http_request_t* req)
{
  DFK_CALL(req->http->dfk, dfk_strmap_free(&req->arguments));
  DFK_CALL(req->http->dfk, dfk_strmap_free(&req->headers));
  return dfk_err_ok;
}


typedef struct dfk_header_parser_data_t {
  dfk_http_request_t* req;
  dfk_strmap_item_t* cheader; /* current header */
  const char* cheader_field;
  const char* cheader_value;
  int dfk_errno;
} dfk_header_parser_data_t;


typedef struct dfk_body_parser_data_t {
  dfk_http_request_t* req;
  dfk_buf_t outbuf;
  int dfk_errno;
} dfk_body_parser_data_t;


static int dfk_http_request_on_message_begin(http_parser* parser)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p}", (void*) p->req);
  return 0;
}


static int dfk_http_request_on_url(http_parser* parser, const char* at, size_t size)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p} %.*s", (void*) p->req, (int) size, at);
  dfk_buf_append(&p->req->url, at, size);
  if (p->req->url.size > DFK_HTTP_HEADER_MAX_SIZE) {
    p->dfk_errno = dfk_err_overflow;
    return 1;
  }
  return 0;
}


static int dfk_http_request_on_header_field(http_parser* parser, const char* at, size_t size)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p} %.*s", (void*) p->req, (int) size, at);
  if (!p->cheader || p->cheader->value.data) {
    if (p->cheader) {
      DFK_CALL(p->req->http->dfk, dfk_strmap_insert(&p->req->headers, p->cheader));
    }
    p->cheader = dfk_arena_alloc(p->req->_request_arena, sizeof(dfk_strmap_item_t));
    if (!p->cheader) {
      p->dfk_errno = dfk_err_nomem;
      return 1;
    }
    DFK_CALL(p->req->http->dfk, dfk_strmap_item_init(p->cheader, NULL, 0, NULL, 0));
    p->cheader_field = at;
    p->cheader_value = 0;
  }
  dfk_buf_append(&p->cheader->key, at, size);
  if (p->cheader->key.size > DFK_HTTP_HEADER_MAX_SIZE) {
    p->dfk_errno = dfk_err_overflow;
    return 1;
  }
  return 0;
}


static int dfk_http_request_on_header_value(http_parser* parser, const char* at, size_t size)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p} %.*s", (void*) p->req, (int) size, at);
  if (!p->cheader_value) {
    p->cheader_value = at;
  }
  dfk_buf_append(&p->cheader->value, at, size);
  if (p->cheader->value.size > DFK_HTTP_HEADER_MAX_SIZE) {
    p->dfk_errno = dfk_err_overflow;
    return 1;
  }
  return 0;
}


static int dfk_http_request_on_headers_complete(http_parser* parser)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p}", (void*) p->req);
  if (p->cheader) {
    DFK_CALL(p->req->http->dfk, dfk_strmap_insert(&p->req->headers, p->cheader));
    p->cheader = NULL;
  }
  http_parser_pause(parser, 1);
  return 0;
}


static int dfk_http_request_on_body(http_parser* parser, const char* at, size_t size)
{
  dfk_body_parser_data_t* p = (dfk_body_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p} %llu bytes", (void*) p->req, (unsigned long long) size);
  memmove(p->outbuf.data, at, size);
  assert(size <= p->outbuf.size);
  p->outbuf.data += size;
  p->outbuf.size -= size;
  p->req->_body_nread += size;
  return 0;
}


static int dfk_http_request_on_message_complete(http_parser* parser)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  /*
   * Use only p->req field below, since a pointer to dfk_body_parser_data_t
   * could be stored in parser->data as well
   */
  DFK_DBG(p->req->http->dfk, "{%p}", (void*) p->req);
  http_parser_pause(parser, 1);
  return 0;
}


static int dfk_http_request_on_chunk_header(http_parser* parser)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p}, chunk size %llu", (void*) p->req,
      (unsigned long long) parser->content_length);
  return 0;
}


static int dfk_http_request_on_chunk_complete(http_parser* parser)
{
  dfk_header_parser_data_t* p = (dfk_header_parser_data_t*) parser->data;
  DFK_DBG(p->req->http->dfk, "{%p}", (void*) p->req);
  return 0;
}


static http_parser_settings dfk_parser_settings = {
  .on_message_begin = dfk_http_request_on_message_begin,
  .on_url = dfk_http_request_on_url,
  .on_status = NULL, /* For HTTP responses only */
  .on_header_field = dfk_http_request_on_header_field,
  .on_header_value = dfk_http_request_on_header_value,
  .on_headers_complete = dfk_http_request_on_headers_complete,
  .on_body = dfk_http_request_on_body,
  .on_message_complete = dfk_http_request_on_message_complete,
  .on_chunk_header = dfk_http_request_on_chunk_header,
  .on_chunk_complete = dfk_http_request_on_chunk_complete
};


int dfk_http_request_read_headers(dfk_http_request_t* req)
{
  assert(req);
  assert(req->http);

  dfk_t* dfk = req->http->dfk;

  /* A pointer to pdata is passed to http_parser callbacks */
  dfk_header_parser_data_t pdata = {
    .req = req,
    .cheader = NULL,
    .cheader_field = NULL,
    .cheader_value = NULL,
    .dfk_errno = dfk_err_ok
  };

  /* Initialize http_parser instance */
  http_parser_init(&req->_parser, HTTP_REQUEST);
  req->_parser.data = &pdata;

  /* Allocate first per-request buffer */
  dfk_buf_t curbuf;
  DFK_CALL(dfk, dfk_http_request_allocate_headers_buf(req, &curbuf));

  while (1) {
    assert(curbuf.size >= DFK_HTTP_HEADER_MAX_SIZE);

    ssize_t nread = dfk_http_request_mocked_read(req, curbuf.data, curbuf.size);
    if (nread <= 0) {
      return dfk_err_eof;
    }

    assert(nread <= (ssize_t) curbuf.size);

    DFK_DBG(dfk, "{%p} http parse bytes: %llu",
        (void*) req, (unsigned long long) nread);

    /*
     * Invoke http_parser.
     * Note that size_t -> ssize_t cast occurs in the line below
     */
    ssize_t nparsed = http_parser_execute(
        &req->_parser, &dfk_parser_settings, curbuf.data, curbuf.size);
    assert(nparsed <= nread);
    DFK_DBG(dfk, "{%p} %llu bytes parsed",
        (void*) req, (unsigned long long) nparsed);
    DFK_DBG(dfk, "{%p} http parser returned %d (%s) - %s",
        (void*) req, req->_parser.http_errno,
        http_errno_name(req->_parser.http_errno),
        http_errno_description(req->_parser.http_errno));

    if (req->_parser.http_errno == HPE_PAUSED) {
      /* on_headers_complete was called */
      req->_remainder = (dfk_buf_t) {curbuf.data + nparsed, nread - nparsed};
      http_parser_pause(&req->_parser, 0);
      break;
    }
    if (HPE_CB_message_begin <= req->_parser.http_errno
        && req->_parser.http_errno <= HPE_CB_chunk_complete) {
      return pdata.dfk_errno;
    }
    if (req->_parser.http_errno != HPE_OK) {
      return dfk_err_protocol;
    }

    if (pdata.cheader) {
      if (curbuf.data + curbuf.size - pdata.cheader_field < DFK_HTTP_HEADER_MAX_SIZE) {
        if (req->_buffers.size == req->http->headers_buffer_count) {
          return dfk_err_overflow;
        }
        DFK_CALL(dfk, dfk_http_request_allocate_headers_buf(req, &curbuf));
        memcpy(curbuf.data, pdata.cheader, nparsed);
        if (pdata.cheader_value) {
          pdata.cheader->value.data += (curbuf.data - pdata.cheader_field);
        }
        assert(pdata.cheader_field);
        pdata.cheader->key.data = curbuf.data;
      }
    }
    curbuf = (dfk_buf_t) {curbuf.data + nparsed, curbuf.size - nparsed};
  }

  /* Request pre-processing */
  DFK_DBG(dfk, "{%p} set version = %d.%d and method = %s",
      (void*) req, req->_parser.http_major, req->_parser.http_minor,
      http_method_str(req->_parser.method));
  req->major_version = req->_parser.http_major;
  req->minor_version = req->_parser.http_minor;
  req->method = req->_parser.method;
  DFK_DBG(dfk, "{%p} populate common headers", (void*) req);
  req->user_agent = dfk_strmap_get(&req->headers, DFK_HTTP_USER_AGENT, sizeof(DFK_HTTP_USER_AGENT) - 1);
  DFK_DBG(dfk, "{%p} user-agent: \"%.*s\"", (void*) req,
      (int) req->user_agent.size, req->user_agent.data);
  req->host = dfk_strmap_get(&req->headers, DFK_HTTP_HOST, sizeof(DFK_HTTP_HOST) - 1);
  DFK_DBG(dfk, "{%p} host: \"%.*s\"", (void*) req,
      (int) req->host.size, req->host.data);
  req->accept = dfk_strmap_get(&req->headers, DFK_HTTP_ACCEPT, sizeof(DFK_HTTP_ACCEPT) - 1);
  DFK_DBG(dfk, "{%p} accept: \"%.*s\"", (void*) req,
      (int) req->accept.size, req->accept.data);
  req->content_type = dfk_strmap_get(&req->headers, DFK_HTTP_CONTENT_TYPE, sizeof(DFK_HTTP_CONTENT_TYPE) - 1);
  DFK_DBG(dfk, "{%p} content-type: \"%.*s\"", (void*) req,
      (int) req->content_type.size, req->content_type.data);
  dfk_buf_t content_length = dfk_strmap_get(&req->headers, DFK_HTTP_CONTENT_LENGTH, sizeof(DFK_HTTP_CONTENT_LENGTH) - 1);
  DFK_DBG(dfk, "{%p} parse content length \"%.*s\"", (void*) req, (int) content_length.size, content_length.data);
  if (content_length.size) {
    long long intval;
    int res = dfk_strtoll(content_length, NULL, 10, &intval);
    if (res != dfk_err_ok) {
      DFK_WARNING(dfk, "{%p} malformed value for \"" DFK_HTTP_CONTENT_LENGTH "\" header: %.*s",
          (void*) req, (int) content_length.size, content_length.data);
    } else {
      req->content_length = (uint64_t) intval;
    }
  }
  req->keepalive = http_should_keep_alive(&req->_parser);
  req->chunked = !!(req->_parser.flags & F_CHUNKED);
  if (req->content_length == (uint64_t) -1 && !req->chunked) {
    DFK_DBG(dfk, "{%p} no content-length header, no chunked encoding - "
        "expecting empty body", (void*) req);
    req->content_length = 0;
  }
  DFK_DBG(dfk, "{%p} keepalive: %d, chunked encoding: %d",
      (void*) req, req->keepalive, req->chunked);

  /** @todo parse url here */
  return dfk_err_ok;
}


ssize_t dfk_http_request_read(dfk_http_request_t* req, char* buf, size_t size)
{
  if (!req) {
    return -1;
  }

  if(!buf && size) {
    req->http->dfk->dfk_errno = dfk_err_badarg;
    return -1;
  }

  DFK_DBG(req->http->dfk, "{%p} bytes requested: %llu",
      (void*) req, (unsigned long long) size);

  if (!req->content_length && !req->chunked) {
    DFK_WARNING(req->http->dfk, "{%p} can not read request body without "
        "content-length or transfer-encoding: chunked", (void*) req);
    req->http->dfk->dfk_errno = dfk_err_eof;
    return -1;
  }

  char* bufcopy = buf;
  size_t sizecopy = size;
  DFK_UNUSED(sizecopy);
  dfk_body_parser_data_t pdata;
  pdata.req = req;
  pdata.dfk_errno = dfk_err_ok;
  pdata.outbuf = (dfk_buf_t) {buf, size};

  while (size && (pdata.outbuf.data == bufcopy || req->_remainder.size)) {
    size_t toread = DFK_MIN(size, req->content_length - req->_body_nread);
    DFK_DBG(req->http->dfk, "{%p} bytes cached: %llu, user-provided buffer used: %llu/%llu bytes",
        (void*) req, (unsigned long long) req->_remainder.size,
        (unsigned long long) (pdata.outbuf.data - bufcopy),
        (unsigned long long) sizecopy);
    DFK_DBG(req->http->dfk, "{%p} will read %llu bytes", (void*) req, (unsigned long long) toread);
    if (size > toread) {
      DFK_WARNING(req->http->dfk, "{%p} requested more bytes than could be read from "
          "request, content-length: %llu, already read: %llu, requested: %llu",
          (void*) req, (unsigned long long) req->content_length,
          (unsigned long long) req->_body_nread,
          (unsigned long long) size);
    }

    dfk_buf_t inbuf;
    /* prepare inbuf - use either bytes cached in req->_remainder,
     * or read from req->_sock
     */
    if (req->_remainder.size) {
      toread = DFK_MIN(toread, req->_remainder.size);
      inbuf = (dfk_buf_t) {req->_remainder.data, toread};
    } else {
      DFK_DBG(req->http->dfk, "{%p} cache is empty, read new bytes", (void*) req);
      size_t nread = dfk_http_request_mocked_read(req, buf, toread);
      if (nread <= 0) {
        /* preserve dfk->dfk_errno from dfk_tcp_socket_read or dfk_sponge_read */
        return nread;
      }
      inbuf = (dfk_buf_t) {buf, nread};
    }

    assert(inbuf.data);
    assert(inbuf.size > 0);
    req->_parser.data = &pdata;
    DFK_DBG(req->http->dfk, "{%p} http parse bytes: %llu",
        (void*) req, (unsigned long long) inbuf.size);
    size_t nparsed = http_parser_execute(
        &req->_parser, &dfk_parser_settings, inbuf.data, inbuf.size);
    DFK_DBG(req->http->dfk, "{%p} %llu bytes parsed",
        (void*) req, (unsigned long long) nparsed);
    DFK_DBG(req->http->dfk, "{%p} http parser returned %d (%s) - %s",
        (void*) req, req->_parser.http_errno,
        http_errno_name(req->_parser.http_errno),
        http_errno_description(req->_parser.http_errno));
    if (req->_parser.http_errno != HPE_PAUSED
        && req->_parser.http_errno != HPE_OK) {
      return dfk_err_protocol;
    }
    assert(nparsed <= inbuf.size);
    if (req->_remainder.size) {
      /* bytes from remainder were parsed */
      req->_remainder.data += nparsed;
      req->_remainder.size -= nparsed;
    }
    buf = pdata.outbuf.data;
    size = pdata.outbuf.size;
  }
  DFK_DBG(req->http->dfk, "%llu", (unsigned long long ) (pdata.outbuf.data - bufcopy));
  return pdata.outbuf.data - bufcopy;
}


ssize_t dfk_http_request_readv(dfk_http_request_t* req, dfk_iovec_t* iov, size_t niov)
{
  if (!req || (!iov && niov)) {
    return dfk_err_badarg;
  }
  DFK_DBG(req->http->dfk, "{%p} into %llu blocks", (void*) req, (unsigned long long) niov);
  return dfk_http_request_read(req, iov[0].data, iov[0].size);
}

