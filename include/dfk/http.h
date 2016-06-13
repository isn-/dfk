/**
 * @file dfk/http.h
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dfk/config.h>
#include <dfk/tcp_socket.h>
#include <dfk/internal/arena.h>
#include <dfk/internal/avltree.h>


typedef enum dfk_http_method_e {
  DFK_HTTP_OPTIONS,
  DFK_HTTP_GET,
  DFK_HTTP_HEAD,
  DFK_HTTP_POST,
  DFK_HTTP_PUT,
  DFK_HTTP_PATCH,
  DFK_HTTP_DELETE,
  DFK_HTTP_TRACE,
  DFK_HTTP_CONNECT
} dfk_http_method_e;


typedef enum dfk_http_status_e {
  DFK_HTTP_CONTINUE = 100,
  DFK_HTTP_SWITCHING_PROTOCOLS = 101,
  DFK_HTTP_PROCESSING = 102, /* WebDAV, RFC 2518 */

  DFK_HTTP_OK = 200,
  DFK_HTTP_CREATED = 201,
  DFK_HTTP_ACCEPTED = 202,
  DFK_HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
  DFK_HTTP_NO_CONTENT = 204 ,
  DFK_HTTP_RESET_CONTENT = 205,
  DFK_HTTP_PARTIAL_CONTENT = 206, /* RFC 7233 */
  DFK_HTTP_MULTI_STATUS = 207, /* WebDAV, RFC 4918 */
  DFK_HTTP_ALREADY_REPORTED = 208, /* WebDAV, RFC 5842 */
  DFK_HTTP_IM_USED = 226, /* RFC 3229 */

  DFK_HTTP_MULTIPLE_CHOICES = 300,
  DFK_HTTP_MOVED_PERMANENTLY = 301,
  DFK_HTTP_FOUND = 302,
  DFK_HTTP_SEE_OTHER = 303,
  DFK_HTTP_NOT_MODIFIED = 304, /* RFC 7232 */
  DFK_HTTP_USE_PROXY = 305,
  DFK_HTTP_SWITCH_PROXY = 306,
  DFK_HTTP_TEMPORARY_REDIRECT = 307,
  DFK_HTTP_PERMANENT_REDIRECT = 308, /* RFC 7538 */

  DFK_HTTP_BAD_REQUEST = 400,
  DFK_HTTP_UNAUTHORIZED = 401, /* RFC 7235 */
  DFK_HTTP_PAYMENT_REQUIRED = 402,
  DFK_HTTP_FORBIDDEN = 403,
  DFK_HTTP_NOT_FOUND = 404,
  DFK_HTTP_METHOD_NOT_ALLOWED = 405,
  DFK_HTTP_NOT_ACCEPTABLE = 406,
  DFK_HTTP_PROXY_AUTHENTICATION_REQUIRED = 407, /* RFC 7235 */
  DFK_HTTP_REQUEST_TIMEOUT = 408,
  DFK_HTTP_CONFLICT = 409,
  DFK_HTTP_GONE = 410,
  DFK_HTTP_LENGTH_REQUIRED = 411,
  DFK_HTTP_PRECONDITION_FAILED = 412, /* RFC 7232 */
  DFK_HTTP_PAYLOAD_TOO_LARGE = 413, /* RFC 7231 */
  DFK_HTTP_URI_TOO_LONG = 414, /* RFC 7231 */
  DFK_HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
  DFK_HTTP_RANGE_NOT_SATISFIABLE = 416, /* RFC 7233 */
  DFK_HTTP_EXPECTATION_FAILED = 417,
  DFK_HTTP_I_AM_A_TEAPOT = 418, /* RFC 2324 */
  DFK_HTTP_MISDIRECTED_REQUEST = 421, /* RFC 7540 */
  DFK_HTTP_UNPROCESSABLE_ENTITY = 422, /* WebDAV; RFC 4918 */
  DFK_HTTP_LOCKED = 423, /* WebDAV; RFC 4918 */
  DFK_HTTP_FAILED_DEPENDENCY = 424, /* WebDAV; RFC 4918 */
  DFK_HTTP_UPGRADE_REQUIRED = 426,
  DFK_HTTP_PRECONDITION_REQUIRED = 428, /* RFC 6585 */
  DFK_HTTP_TOO_MANY_REQUESTS = 429, /* RFC 6585 */
  DFK_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431, /* RFC 6585 */
  DFK_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

  DFK_HTTP_INTERNAL_SERVER_ERROR = 500,
  DFK_HTTP_NOT_IMPLEMENTED = 501,
  DFK_HTTP_BAD_GATEWAY = 502,
  DFK_HTTP_SERVICE_UNAVAILABLE = 503,
  DFK_HTTP_GATEWAY_TIMEOUT = 504,
  DFK_HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
  DFK_HTTP_VARIANT_ALSO_NEGOTIATES = 506, /* RFC 2295 */
  DFK_HTTP_INSUFFICIENT_STORAGE = 507, /* WebDAV; RFC 4918 */
  DFK_HTTP_LOOP_DETECTED = 508, /* WebDAV; RFC 5842 */
  DFK_HTTP_NOT_EXTENDED = 510, /* RFC 2774 */
  DFK_HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511 /* RFC 6585 */
} dfk_http_status_e;


typedef struct dfk_http_header_t {
  struct {
    dfk_avltree_hook_t hook;
  } _;
  dfk_buf_t name;
  dfk_buf_t value;
} dfk_http_header_t;


typedef dfk_http_header_t dfk_http_argument_t;


typedef struct dfk_http_req_t {
  struct {
    dfk_arena_t* arena;
    dfk_tcp_socket_t* sock;
  } _;
  unsigned short http_major;
  unsigned short http_minor;
  dfk_http_method_e method;
  dfk_buf_t url;
  dfk_avltree_t headers; /* contains dfk_http_header_t */
  dfk_avltree_t arguments; /* contains dfk_http_argument_t */
} dfk_http_req_t;


typedef struct dfk_http_resp_t {
  struct {
    dfk_arena_t* arena;
    dfk_tcp_socket_t* sock;
    dfk_list_t headers;
  } _;
  dfk_http_status_e code;
} dfk_http_resp_t;


ssize_t dfk_http_read(dfk_http_req_t* req, const char* buf, size_t size);
ssize_t dfk_http_readv(dfk_http_req_t* req, dfk_iovec_t* iov, size_t niov);

ssize_t dfk_http_write(dfk_http_resp_t* resp, char* buf, size_t nbtes);
ssize_t dfk_http_writev(dfk_http_resp_t* resp, dfk_iovec_t* iov, size_t niov);

void dfk_http_set(dfk_http_resp_t* resp, const char* name, size_t namesize, size_t size, const char* fmt, ...);

typedef int (*dfk_http_handler)(dfk_t*, dfk_http_req_t*, dfk_http_resp_t*);


typedef struct dfk_http_t {
  struct {
    dfk_tcp_socket_t listensock;
    dfk_http_handler handler;
  } _;
  dfk_t* dfk;
} dfk_http_t;


int dfk_http_init(dfk_http_t* http, dfk_t* dfk);
int dfk_http_free(dfk_http_t* http);
int dfk_http_serve(dfk_http_t* http,
    const char* endpoint,
    uint16_t port,
    dfk_http_handler handler);

