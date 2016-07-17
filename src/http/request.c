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
#include <dfk/http/request.h>
#include <dfk/internal.h>


void dfk__http_request_init(dfk_http_request_t* req, dfk_t* dfk,
                            dfk_arena_t* request_arena,
                            dfk_arena_t* connection_arena,
                            dfk_tcp_socket_t* sock)
{
  assert(req);
  assert(request_arena);
  assert(connection_arena);
  assert(sock);
  req->_request_arena = request_arena;
  req->_connection_arena = connection_arena;
  req->_sock = sock;
  dfk_avltree_init(&req->_headers, dfk__http_headers_cmp);
  dfk_avltree_init(&req->_arguments, dfk__http_headers_cmp);
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


void dfk__http_request_free(dfk_http_request_t* req)
{
  dfk_avltree_free(&req->_arguments);
  dfk_avltree_free(&req->_headers);
}


ssize_t dfk_http_read(dfk_http_request_t* req, char* buf, size_t size)
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


ssize_t dfk_http_readv(dfk_http_request_t* req, dfk_iovec_t* iov, size_t niov)
{
  if (!req || (!iov && niov)) {
    return dfk_err_badarg;
  }
  DFK_DBG(req->dfk, "{%p} into %llu blocks", (void*) req, (unsigned long long) niov);
  return dfk_http_read(req, iov[0].data, iov[0].size);
}


dfk_buf_t dfk_http_request_get(struct dfk_http_request_t* req,
                               const char* name, size_t namesize)
{
  if (!req) {
    return (dfk_buf_t) {NULL, 0};
  }
  if (!name && namesize) {
    req->dfk->dfk_errno = dfk_err_badarg;
    return (dfk_buf_t) {NULL, 0};
  }
  return dfk__http_headers_get(&req->_headers, name, namesize);
}


int dfk_http_request_headers_begin(struct dfk_http_request_t* req,
                                   dfk_http_header_it* it)
{
  if (!req || !it) {
    return dfk_err_badarg;
  }
  return dfk__http_headers_begin(&req->_headers, it);
}

