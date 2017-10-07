/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <dfk/internal.h>
#include <dfk/error.h>
#include <dfk/http/protocol.h>
#include <dfk/internal/http/request.h>
#include <dfk/internal/http/response.h>

void dfk__http_protocol(dfk_http_t* http, dfk_fiber_t* fiber, dfk_tcp_socket_t* sock,
    dfk_http_handler handler, dfk_userdata_t user)
{
  assert(http);
  assert(fiber);
  assert(sock);
  assert(handler);

  dfk_t* dfk = fiber->dfk;

  /*
  struct linger l = {
    .l_onoff = 1,
    .l_linger = 10
  };
  setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
  */

  /* Arena for per-connection data */
  dfk_arena_t connection_arena;
  dfk_arena_init(&connection_arena, http->dfk);
  DFK_DBG(dfk, "{%p} initialize connection arena %p",
      (void*) sock, (void*) &connection_arena);

  /* Requests processed within this connection */
  ssize_t nrequests = 0;
  int keepalive = 1;

  while (keepalive) {
    /* Arena for per-request data */
    dfk_arena_t request_arena;
    dfk_arena_init(&request_arena, http->dfk);
    DFK_DBG(dfk, "{%p} initialize request arena %p",
        (void*) sock, (void*) &request_arena);

    dfk_http_request_t req;
    /** @todo check return value */
    dfk__http_request_init(&req, http, &request_arena, &connection_arena, sock);

    int err = dfk__http_request_read_headers(&req);
    if (err != dfk_err_ok) {
      DFK_ERROR(dfk, "{%p} dfk__http_request_read_headers failed with %s",
          (void*) http, dfk_strerr(dfk, err));
      keepalive = 0;
      goto cleanup;
    }

    DFK_DBG(http->dfk, "{%p} request parsing done, "
        "%s %.*s HTTP/%hu.%hu \"%.*s\"",
        (void*) http,
        http_method_str((enum http_method) req.method),
        (int) req.url.size, req.url.data,
        req.major_version,
        req.minor_version,
        (int) req.user_agent.size, req.user_agent.data);

    /* Determine requested connection type */
    dfk_buf_t connection = dfk_strmap_get(&req.headers,
        DFK_HTTP_CONNECTION, sizeof(DFK_HTTP_CONNECTION) - 1);
    if (connection.size) {
      if (!strncmp(connection.data, "close", DFK_MIN(connection.size, 5))) {
        keepalive = 0;
      }
      if (!strncmp(connection.data, "Keep-Alive", DFK_MIN(connection.size, 10))) {
        keepalive = 1;
      }
    } else {
      keepalive = !(req.major_version == 1 && req.minor_version == 0);
    }
    DFK_DBG(http->dfk, "{%p} client requested %skeepalive connection",
        (void*) http, keepalive ? "" : "not ");

    dfk_http_response_t resp;
    /** @todo check return value */
    dfk__http_response_init(&resp, &req, &request_arena, &connection_arena, sock, keepalive);

    DFK_DBG(http->dfk, "{%p} run request handler", (void*) http);
    int hres = handler(http, &req, &resp, user);
    DFK_INFO(http->dfk, "{%p} http handler returned %s",
        (void*) http, dfk_strerr(http->dfk, hres));
    if (hres != dfk_err_ok) {
      resp.status = DFK_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* Fix request handler possible protocol violations */
    if (req.major_version < resp.major_version) {
      DFK_WARNING(http->dfk,
          "{%p} request handler attempted HTTP version upgrade from %d.%d to %d.%d",
          (void*) http, req.major_version, req.minor_version,
          resp.major_version, resp.minor_version);
      resp.major_version = req.major_version;
      resp.minor_version = req.minor_version;
    } else if (req.minor_version < resp.minor_version) {
      DFK_WARNING(http->dfk,
          "{%p} request handler attempted HTTP version upgrade from %d.%d to %d.%d",
          (void*) http, req.major_version, req.minor_version,
          resp.major_version, resp.minor_version);
      resp.minor_version = req.minor_version;
    }

    if (!keepalive && resp.keepalive) {
      DFK_WARNING(http->dfk, "{%p} request handler enabled keepalive, although "
          "client have not requested", (void*) http);
    }

    /*
     * Keep connection alive only if it was requested by the client and request
     * handler did not refuse.
     */
    keepalive = keepalive && resp.keepalive;

    if (keepalive
        && http->keepalive_requests >= 0
        && nrequests + 1 >= http->keepalive_requests) {
      DFK_INFO(http->dfk, "{%p} maximum number of keepalive requests (%llu) "
          "for connection {%p} has reached, close connection",
          (void*) http, (unsigned long long) http->keepalive_requests,
          (void*) sock);
      keepalive = 0;
    }

#if DFK_DEBUG
    {
      dfk_buf_t connection = dfk_strmap_get(&req.headers,
          DFK_HTTP_CONNECTION, sizeof(DFK_HTTP_CONNECTION) - 1);
      if (connection.size) {
        DFK_WARNING(http->dfk, "{%p} manually set header \""
            DFK_HTTP_CONNECTION " : %.*s\" will be overwritten"
            " use dfk_http_response_t.keepalive instead",
            (void*) http, (int) connection.size, connection.data);
      }
    }
#endif

    if (!keepalive) {
      dfk_http_response_set(&resp,
          DFK_HTTP_CONNECTION, sizeof(DFK_HTTP_CONNECTION) - 1, "close", 5);
    } else {
      dfk_http_response_set(&resp,
          DFK_HTTP_CONNECTION, sizeof(DFK_HTTP_CONNECTION) - 1, "Keep-Alive", 10);
    }

    /** @todo remove kludge */
    dfk_http_response_set(&resp, "Content-Length", 14, "0", 1);

    dfk__http_response_flush_headers(&resp);

    /*
     * If request handler hasn't read all bytes of the body, we have to
     * skip them at this point.
     */
    if (req.content_length > 0 && (int64_t) req._body_nread < req.content_length) {
      size_t bytesremain = req.content_length - req._body_nread;
      DFK_DBG(dfk, "{%p} bytes read by handler %llu, need to flush %llu",
          (void*) http, (unsigned long long ) req._body_nread,
          (unsigned long long) bytesremain);
      char buf[1024];
      while (bytesremain) {
        ssize_t nread = dfk_http_request_read(&req, buf,
            DFK_MIN(bytesremain, sizeof(buf)));
        if (nread < 0) {
          DFK_ERROR(dfk, "{%p} failed to flush request body", (void*) http);
          keepalive = 0;
          goto cleanup;
        }
        bytesremain -= nread;
      }
    }

    DFK_INFO(http->dfk,
        "{%p} %s %.*s HTTP/%hu.%hu \"%.*s\"",
        (void*) http,
        http_method_str((enum http_method) req.method),
        (int) req.url.size, req.url.data,
        req.major_version,
        req.minor_version,
        (int) req.user_agent.size, req.user_agent.data);

    ++nrequests;

cleanup:
    DFK_DBG(dfk, "{%p} cleaup per-request resources", (void*) http);
    dfk__http_request_free(&req);
    dfk_arena_free(&request_arena);
  }

  dfk_arena_free(&connection_arena);

  /*
  DFK_DBG(dfk, "{%p} wait for client to close connection", (void*) http);
  DFK_CALL_RVOID(dfk, dfk_tcp_socket_shutdown(sock, DFK_SHUT_RDWR));
  int err = dfk_tcp_socket_wait_disconnect(sock, 1000);
  if (err == dfk_err_timeout) {
    DFK_WARNING(dfk, "{%p} client has not closed connection, force close", (void*) http);
  }
  if (err != dfk_err_ok) {
    DFK_ERROR(dfk, "{%p} dfk_tcp_socket_wait_disconnect returned %s",
        (void*) http, dfk_strerr(dfk, err));
  }
  */
  DFK_DBG(http->dfk, "{%p} close socket", (void*) http);
  DFK_CALL_RVOID(http->dfk, dfk_tcp_socket_close(sock));
}

