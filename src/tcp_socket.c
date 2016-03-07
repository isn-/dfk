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
#include <dfk/config.h>
#include <dfk/tcp_socket.h>
#include "common.h"

#define LOOP(sock) ((dfk_event_loop_t*) (sock)->_.socket.loop->data)
#define CTX(sock) (LOOP(sock)->_.ctx)

/* TCP Socket states
 * (*)        -> spare
 * spare      -> listening
 *            -> connecting
 * listening  -> spare
 *            -> closing
 * connecting -> connected
 *            -> spare (connection failed)
 * connected  -> reading
 *            -> writing
 *            -> closing
 * reading    -> connected
 *            -> closing
 * writing    -> connected
 *            -> closing
 * closing    -> closed
 * closed     ->
 */
#define TCP_SOCKET_SPARE ((int) 1)
#define TCP_SOCKET_LISTENING ((int) 1 << 1)
#define TCP_SOCKET_CONNECTING ((int) 1 << 2)
#define TCP_SOCKET_CONNECTED ((int) 1 << 3)
#define TCP_SOCKET_READING ((int) 1 << 4)
#define TCP_SOCKET_WRITING ((int) 1 << 5)
#define TCP_SOCKET_CLOSING ((int) 1 << 6)
#define TCP_SOCKET_CLOSED ((int) 1 << 7)

#define STATE(sock) ((sock)->_.flags & 0xFF)
#define TO_STATE(sock, state) ((sock)->_.flags = ((sock)->_.flags & 0xFFFFFF00) | (state))

/* Accepted socket has an accociated coroutine and hence
 * requires explicit coroutine join upon closing
 */
#define TCP_SOCKET_IS_ACCEPTED ((int) 1 << 8)
#define TCP_SOCKET_CONNECT_PENDING ((int) 1 << 9)
#define TCP_SOCKET_IS_LISTENING ((int) 1 << 10)

#define FLAG(sock, flag) (((sock)->_.flags & 0xFFFFFF00) & (flag))


int dfk_tcp_socket_init(dfk_tcp_socket_t* sock, dfk_event_loop_t* loop)
{
  int err;
  if (sock == NULL || loop == NULL) {
    return dfk_err_badarg;
  }
  DFK_DEBUG(loop->_.ctx, "(%p)", (void*) sock);
  err = uv_tcp_init(&loop->_.loop, &sock->_.socket);
  sock->_.socket.data = sock;
  if (err != 0) {
    DFK_ERROR(loop->_.ctx, "(%p) uv_tcp_init returned %d", (void*) sock, err);
    loop->_.ctx->sys_errno = err;
    return dfk_err_sys;
  }
  sock->_.flags = TCP_SOCKET_SPARE;
  return dfk_err_ok;
}


int dfk_tcp_socket_free(dfk_tcp_socket_t* sock)
{
  int err;
  if (sock == NULL) {
    return dfk_err_badarg;
  }
  DFK_DEBUG(CTX(sock), "(%p)", (void*) sock);
  if (STATE(sock) != TCP_SOCKET_SPARE) {
    if ((err = dfk_tcp_socket_close(sock)) != dfk_err_ok) {
      return err;
    }
  }
  if (sock->_.flags & TCP_SOCKET_CONNECT_PENDING) {
    DFK_FREE(CTX(sock), sock->_.arg.obj);
  }
  return dfk_err_ok;
}


typedef struct _start_connect_async_arg_t {
  uv_connect_t connect;
  void (*callback)(dfk_tcp_socket_t*);
  dfk_coro_t* coro;
} _start_connect_async_arg_t;


static void dfk_tcp_socket_on_connect_1(uv_connect_t* p, int status)
{
  dfk_tcp_socket_t* sock = (dfk_tcp_socket_t*) p->data;
  _start_connect_async_arg_t* arg;
  dfk_coro_t* coro;
  int err;

  assert(p);
  assert(sock);
  assert(STATE(sock) == TCP_SOCKET_CONNECTING);

  arg = (_start_connect_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  assert(FLAG(sock, TCP_SOCKET_CONNECT_PENDING));
  sock->_.flags ^= TCP_SOCKET_CONNECT_PENDING;

  err = dfk_coro_run(arg->coro, (void (*)(void*)) arg->callback, sock);

  coro = arg->coro;
  DFK_FREE(CTX(sock), arg);
  sock->_.arg.obj = NULL;

  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "dfk_coro_run returned %d", err);
    return;
  }

  if (status != 0) {
    CTX(sock)->sys_errno = status;
    TO_STATE(sock, TCP_SOCKET_SPARE);
  } else {
    TO_STATE(sock, TCP_SOCKET_CONNECTED);
  }

  if ((err = dfk_coro_yield_to(CTX(sock), coro)) != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "dfk_coro_yield_to returned %d", err);
  }
}


int dfk_tcp_socket_start_connect(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_tcp_socket_t*),
    dfk_coro_t* coro)
{
  int err;
  struct sockaddr_in dest;
  _start_connect_async_arg_t* arg = NULL;

  if (sock == NULL || endpoint == NULL || callback == NULL) {
    return dfk_err_badarg;
  }
  if (STATE(sock) != TCP_SOCKET_SPARE) {
    return dfk_err_inprog;
  }
  if (CTX(sock)->_.current_coro != NULL) {
    return dfk_err_coro;
  }

  DFK_INFO(CTX(sock), "(%p) will connect to %s:%u",
      (void*) sock,
      endpoint,
      port);
  if ((err = uv_ip4_addr(endpoint, port, &dest)) != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }
  arg = DFK_MALLOC(CTX(sock), sizeof(_start_connect_async_arg_t));
  if (arg == NULL) {
    return dfk_err_nomem;
  }
  arg->connect.data = sock;
  arg->callback = callback;
  arg->coro = coro;
  sock->_.arg.obj = arg;
  if ((err = uv_tcp_connect(
      &arg->connect,
      &sock->_.socket,
      (const struct sockaddr*) &dest,
      dfk_tcp_socket_on_connect_1)) != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }
  TO_STATE(sock, TCP_SOCKET_CONNECTING);
  sock->_.flags |= TCP_SOCKET_CONNECT_PENDING;
  return dfk_err_ok;
}


typedef struct _connect_async_arg_t {
  int err;
  dfk_coro_t* yieldback;
} _connect_async_arg_t;


static void dfk_tcp_socket_on_connect_2(uv_connect_t* p, int status)
{
  int err;
  dfk_tcp_socket_t* sock = (dfk_tcp_socket_t*) p->data;
  _connect_async_arg_t* arg;

  assert(p);
  assert(sock);
  assert(STATE(sock) == TCP_SOCKET_CONNECTING);

  arg = (_connect_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  sock->_.arg.obj = NULL;
  if (status != 0) {
    CTX(sock)->sys_errno = status;
    arg->err = dfk_err_sys;
  } else {
    arg->err = dfk_err_ok;
  }
  err = dfk_coro_yield_to(CTX(sock), arg->yieldback);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
  }
}


int dfk_tcp_socket_connect(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port)
{
  int err = 0;
  struct sockaddr_in dest;
  uv_connect_t connect;
  _connect_async_arg_t arg;

  if (sock == NULL || endpoint == NULL) {
    return dfk_err_badarg;
  }
  if (STATE(sock) == TCP_SOCKET_CONNECTING) {
    return dfk_err_inprog;
  }
  if (STATE(sock) != TCP_SOCKET_SPARE) {
    return dfk_err_badarg;
  }

  assert(CTX(sock) && LOOP(sock));

  if (CTX(sock)->_.current_coro == NULL) {
    return dfk_err_coro;
  }

  if ((err = uv_ip4_addr(endpoint, port, &dest)) != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  arg.err = dfk_err_ok;
  arg.yieldback = CTX(sock)->_.current_coro;
  assert(sock->_.arg.obj == NULL);
  sock->_.arg.obj = &arg;

  if ((err = uv_tcp_connect(
      &connect,
      &sock->_.socket,
      (const struct sockaddr*) &dest,
      dfk_tcp_socket_on_connect_2)) != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }
  TO_STATE(sock, TCP_SOCKET_CONNECTING);
  if ((err = dfk_coro_yield_to(CTX(sock), &LOOP(sock)->_.coro)) != dfk_err_ok) {
    return err;
  }
  assert(sock->_.arg.obj == NULL);
  if (arg.err == dfk_err_ok) {
    TO_STATE(sock, TCP_SOCKET_CONNECTED);
  } else {
    TO_STATE(sock, TCP_SOCKET_SPARE);
  }
  return arg.err;
}


typedef struct _accepted_socket_t {
  dfk_tcp_socket_t socket;
  dfk_coro_t coro;
} _accepted_socket_t;


static void dfk_tcp_socket_accepted_main_1(void* arg)
{
  int err;
  dfk_tcp_socket_t* sock;
  void (*callback)(dfk_tcp_socket_t*, int);

  assert(arg);
  sock = &((_accepted_socket_t*) arg)->socket;
  assert(sock);
  sock->_.flags |= TCP_SOCKET_IS_ACCEPTED;
  callback = (void(*)(dfk_tcp_socket_t*, int)) sock->_.arg.func;
  assert(callback);

  callback(sock, dfk_err_ok);

  /* coroutine will be joined in the dfk_tcp_socket_on_close */
  if ((err = dfk_tcp_socket_free(sock)) != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_socket_free returned %d", (void*) sock, err);
  }
}


static void dfk_tcp_socket_on_new_connection_1(uv_stream_t* p, int status)
{
  int err;
  dfk_tcp_socket_t* sock;
  _accepted_socket_t* newsock;
  void (*callback)(dfk_tcp_socket_t*);

  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  callback = (void(*)(dfk_tcp_socket_t*)) sock->_.arg.func;
  assert(callback);
  if (status != 0) {
    DFK_ERROR(CTX(sock), "new connection returned %d", status);
    CTX(sock)->sys_errno = status;
    callback(sock);
    return;
  }

  newsock = DFK_MALLOC(CTX(sock), sizeof(_accepted_socket_t));

  if (newsock == NULL) {
    callback(sock);
    return;
  }

  if ((err = dfk_tcp_socket_init(&newsock->socket, LOOP(sock))) != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    callback(sock);
    return;
  }

  if ((err = uv_accept(p, (uv_stream_t*) &newsock->socket._.socket)) != 0) {
    DFK_ERROR(CTX(sock), "accept failed with code %d", err);
    DFK_FREE(CTX(sock), newsock);
    CTX(sock)->sys_errno = err;
    callback(sock);
    return;
  }

  newsock->socket._.arg.func = (void(*)(void*)) callback;
  err = dfk_coro_init(&newsock->coro, CTX(sock), 0);
  if (err != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    CTX(sock)->sys_errno = err;
    callback(sock);
    return;
  }

  err = dfk_coro_run(&newsock->coro, dfk_tcp_socket_accepted_main_1, newsock);
  if (err != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    CTX(sock)->sys_errno = err;
    callback(sock);
    return;
  }

  err = dfk_coro_yield_to(CTX(sock), &newsock->coro);
  if (err != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    CTX(sock)->sys_errno = err;
    callback(sock);
    return;
  }
}


int dfk_tcp_socket_start_listen(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_tcp_socket_t*),
    size_t backlog)
{
  struct sockaddr_in bind;
  int err;

  if (sock == NULL || endpoint == NULL || callback == NULL) {
    return dfk_err_badarg;
  }

  if (STATE(sock) != TCP_SOCKET_SPARE) {
    return dfk_err_badarg;
  }

  if ((err = uv_ip4_addr(endpoint, port, &bind)) != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  if ((err = uv_tcp_bind(&sock->_.socket, (struct sockaddr*) &bind, 0))) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  if (backlog == 0) {
    backlog = DFK_DEFAULT_TCP_BACKLOG;
  }

  assert(sock->_.arg.func == NULL);
  sock->_.arg.func = (void (*)(void*)) callback;

  err = uv_listen(
    (uv_stream_t*) &sock->_.socket,
    backlog,
    dfk_tcp_socket_on_new_connection_1);

  if (err != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  TO_STATE(sock, TCP_SOCKET_LISTENING);
  DFK_INFO(CTX(sock), "(%p) will listen on %s:%u, backlog %lu",
    (void*) sock, endpoint, port, (unsigned long) backlog);
  return dfk_err_ok;
}


typedef struct _listen_async_arg_t {
  dfk_coro_t* yieldback;
  void (*callback)(dfk_tcp_socket_t*);
  int err;
} _listen_async_arg_t;


static void dfk_tcp_socket_accepted_main_2(void* parg)
{
  int err;
  _accepted_socket_t* arg = (_accepted_socket_t*) parg;
  dfk_tcp_socket_t* sock;
  void (*callback)(dfk_tcp_socket_t*);

  assert(arg);
  sock = &arg->socket;
  assert(sock);
  sock->_.flags |= TCP_SOCKET_IS_ACCEPTED;
  callback = (void(*)(dfk_tcp_socket_t*)) sock->_.arg.func;
  assert(callback);

  callback(sock);

  /* coroutine will be joined in the dfk_tcp_socket_on_close */
  if ((err = dfk_tcp_socket_free(sock)) != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_socket_free returned %d", (void*) sock, err);
  }
}


static void dfk_tcp_socket_on_new_connection_2(uv_stream_t* p, int status)
{
  int err;
  dfk_tcp_socket_t* sock;
  _accepted_socket_t* newsock;
  _listen_async_arg_t* arg;

  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (_listen_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  sock->_.arg.obj = NULL;
  if (status != 0) {
    DFK_ERROR(CTX(sock), "new connection returned %d", status);
    CTX(sock)->sys_errno = status;
    arg->err = dfk_err_sys;
    return;
  }

  newsock = DFK_MALLOC(CTX(sock), sizeof(_accepted_socket_t));

  if (newsock == NULL) {
    arg->err = dfk_err_nomem;
    return;
  }

  if ((err = dfk_tcp_socket_init(&newsock->socket, LOOP(sock))) != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    arg->err = err;
    return;
  }

  if ((err = uv_accept(p, (uv_stream_t*) &newsock->socket._.socket)) != 0) {
    DFK_ERROR(CTX(sock), "accept failed with code %d", err);
    DFK_FREE(CTX(sock), newsock);
    CTX(sock)->sys_errno = err;
    arg->err = err;
    return;
  }

  newsock->socket._.arg.func = (void(*)(void*)) arg->callback;
  err = dfk_coro_run(
      &newsock->coro,
      dfk_tcp_socket_accepted_main_2,
      newsock);

  if (err != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    arg->err = err;
    return;
  }

  if ((err = dfk_coro_yield_to(CTX(sock), &newsock->coro)) != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    arg->err = err;
    return;
  }
}

int dfk_tcp_socket_listen(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_tcp_socket_t*),
    size_t backlog)
{
  int err;
  struct sockaddr_in bind;
  _listen_async_arg_t arg;

  if (sock == NULL || endpoint == NULL || callback == NULL) {
    return dfk_err_badarg;
  }

  if (STATE(sock) != TCP_SOCKET_SPARE) {
    return dfk_err_badarg;
  }

  if ((err = uv_ip4_addr(endpoint, port, &bind)) != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  if ((err = uv_tcp_bind(&sock->_.socket, (struct sockaddr*) &bind, 0))) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  if (backlog == 0) {
    backlog = DFK_DEFAULT_TCP_BACKLOG;
  }

  assert(sock->_.arg.func == NULL);

  err = uv_listen(
    (uv_stream_t*) &sock->_.socket,
    backlog,
    dfk_tcp_socket_on_new_connection_2);

  if (err != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  TO_STATE(sock, TCP_SOCKET_LISTENING);
  DFK_INFO(CTX(sock), "(%p) listen on %s:%u, backlog %lu",
    (void*) sock, endpoint, port, (unsigned long) backlog);

  assert(sock->_.arg.obj == NULL);
  arg.yieldback = CTX(sock)->_.current_coro;
  arg.err = dfk_err_ok;
  arg.callback = callback;
  sock->_.arg.obj = &arg;

  if ((err = dfk_coro_yield_to(CTX(sock), &LOOP(sock)->_.coro)) != dfk_err_ok) {
    return err;
  }

  return arg.err;
}

static void dfk_tcp_socket_on_close(uv_handle_t* p)
{
  int err;
  dfk_tcp_socket_t* sock;
  dfk_coro_t* callback;
  dfk_context_t* ctx;
  _listen_async_arg_t* arg;

  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  assert(LOOP(sock) && CTX(sock));
  assert(STATE(sock) & TCP_SOCKET_CLOSING);
  callback = (dfk_coro_t*) sock->_.arg.obj;
  assert(callback);
  sock->_.arg.obj = NULL;
  DFK_INFO(CTX(sock), "(%p) is now closed", (void*) sock);
  ctx = CTX(sock);
  if (FLAG(sock, TCP_SOCKET_IS_ACCEPTED)) {
    if ((err = dfk_coro_join(&((_accepted_socket_t*) sock)->coro)) != dfk_err_ok) {
      DFK_ERROR(CTX(sock), "(%p) dfk_coro_join returned %d", (void*) sock, err);
    }
    DFK_FREE(CTX(sock), sock);
  } else if (FLAG(sock, TCP_SOCKET_LISTENING)) {
    arg = (_listen_async_arg_t*) sock->_.arg.obj;
    err = dfk_coro_yield_to(CTX(sock), arg->yieldback);
    if (err != dfk_err_ok) {
      DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
    }
  }
  err = dfk_coro_yield_to(ctx, callback);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
  }
}

int dfk_tcp_socket_close(dfk_tcp_socket_t* sock)
{
  int err;
  if (sock == NULL) {
    return dfk_err_badarg;
  }
  if (STATE(sock) & TCP_SOCKET_CLOSING) {
    return dfk_err_inprog;
  }
  if (STATE(sock) == TCP_SOCKET_CLOSED) {
    return dfk_err_ok;
  }
  assert(CTX(sock) && LOOP(sock));
  assert(sock->_.arg.obj == NULL);
  sock->_.arg.obj = (void*) CTX(sock)->_.current_coro;
  sock->_.flags |= TCP_SOCKET_CLOSING;
  DFK_INFO(CTX(sock), "(%p) start close", (void*) sock);
  uv_close((uv_handle_t*) &sock->_.socket, dfk_tcp_socket_on_close);
  if ((err = dfk_coro_yield_to(CTX(sock), &LOOP(sock)->_.coro)) != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
    return err;
  }
  assert(sock->_.arg.obj == NULL);
  assert(STATE(sock) & TCP_SOCKET_CLOSING);
  TO_STATE(sock, TCP_SOCKET_CLOSED);
  return dfk_err_ok;
}


typedef struct _read_async_arg_t {
  dfk_coro_t* yieldback;
  size_t nbytes;
  char* buf;
  int err;
} _read_async_arg_t;


static void dfk_tcp_socket_on_read(uv_stream_t* p, ssize_t nread, const uv_buf_t* buf)
{
  int err;
  _read_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  (void) buf;
  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (_read_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  sock->_.arg.obj = NULL;
  if (nread < 0) {
    err = dfk_tcp_socket_close(sock);
    if (err != dfk_err_ok) {
      DFK_ERROR(CTX(sock), "(%p) dfk_tcp_socket_close returned %d", (void*) sock, err);
    }
    arg->err = dfk_err_sys;
  } else {
    arg->nbytes = nread;
    arg->err = dfk_err_ok;
    if ((err = uv_read_stop(p)) != 0) {
      arg->err = dfk_err_sys;
      CTX(sock)->sys_errno = err;
    }
  }

  err = dfk_coro_yield_to(CTX(sock), arg->yieldback);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
  }
}

static void dfk_tcp_socket_on_alloc(uv_handle_t* p, size_t hint, uv_buf_t* buf)
{
  _read_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  (void) hint;
  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (_read_async_arg_t*) sock->_.arg.obj;
  assert(arg);

  DFK_DEBUG(CTX(sock), "(%p) %lu bytes requested", (void*) sock, (unsigned long) hint);

  if (arg->nbytes) {
    *buf = uv_buf_init(arg->buf, arg->nbytes);
  }
}


int dfk_tcp_socket_read(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes,
    size_t* nread)
{
  int err;
  _read_async_arg_t arg;

  if (sock == NULL || buf == NULL || nread == NULL) {
    return dfk_err_badarg;
  }
  DFK_DEBUG(CTX(sock), "(%p) read upto %lu bytes to %p",
      (void*) sock, (unsigned long) nbytes, (void*) buf);
  if (STATE(sock) & TCP_SOCKET_READING) {
    return dfk_err_inprog;
  }
  sock->_.flags |= TCP_SOCKET_READING;

  assert(sock->_.arg.obj == NULL);
  arg.yieldback = LOOP(sock)->_.ctx->_.current_coro;
  arg.nbytes = nbytes;
  arg.buf = buf;
  arg.err = dfk_err_ok;
  sock->_.arg.obj = &arg;

  err = uv_read_start(
      (uv_stream_t*) &sock->_.socket,
      dfk_tcp_socket_on_alloc,
      dfk_tcp_socket_on_read);

  if (err != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  if ((err = dfk_coro_yield_to(CTX(sock), &LOOP(sock)->_.coro)) != dfk_err_ok) {
    return err;
  }

  assert(sock->_.arg.obj == NULL);
  assert(STATE(sock) & TCP_SOCKET_READING);
  sock->_.flags ^= TCP_SOCKET_READING;

  if (arg.err != dfk_err_ok) {
    return arg.err;
  }

  *nread = arg.nbytes;
  return dfk_err_ok;
}

int dfk_tcp_socket_readall(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes,
    size_t* nread)
{
  int err;
  size_t toread = nbytes;

  if (sock == NULL || buf == NULL || nread == NULL) {
    return dfk_err_badarg;
  }
  DFK_DEBUG(CTX(sock), "(%p) read exactly %lu bytes to %p",
      (void*) sock, (unsigned long) nbytes, (void*) buf);

  while (toread > 0) {
    if ((err = dfk_tcp_socket_read(sock, buf, toread, nread)) != dfk_err_ok) {
      *nread = nbytes - toread;
      return err;
    }
    assert(*nread <= toread);
    buf += *nread;
    toread -= *nread;
  }
  assert(toread == 0);
  *nread = nbytes;
  return dfk_err_ok;
}

int dfk_tcp_socket_readv(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov,
    size_t *nread)
{
  (void) sock;
  (void) iov;
  (void) niov;
  (void) nread;
  return dfk_err_ok;
}


typedef struct _write_async_arg_t {
  dfk_coro_t* yieldback;
  int err;
} _write_async_arg_t;


static void dfk_tcp_socket_on_write(uv_write_t* request, int status)
{
  int err;
  _write_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  assert(request);
  sock = (dfk_tcp_socket_t*) request->data;
  assert(sock);
  arg = (_write_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  sock->_.arg.obj = NULL;

  if (status < 0) {
    CTX(sock)->sys_errno = status;
    arg->err = dfk_err_sys;
    err = dfk_tcp_socket_close(sock);
    if (err != dfk_err_ok) {
      DFK_ERROR(CTX(sock), "(%p) dfk_tcp_socket_close returned %d", (void*) sock, err);
    }
  } else {
    arg->err = dfk_err_ok;
  }

  err = dfk_coro_yield_to(CTX(sock), arg->yieldback);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
  }
}

int dfk_tcp_socket_write(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes)
{
  int err;
  uv_write_t request;
  _write_async_arg_t arg;
  uv_buf_t uvbuf = uv_buf_init(buf, nbytes);

  if (sock == NULL || (buf == NULL && nbytes != 0)) {
    return dfk_err_badarg;
  }

  assert(CTX(sock) && LOOP(sock));

  if (STATE(sock) & TCP_SOCKET_WRITING) {
    return dfk_err_inprog;
  }

  if (!(STATE(sock) & TCP_SOCKET_CONNECTED)) {
    return dfk_err_badarg;
  }

  if (nbytes == 0) {
    return dfk_err_ok;
  }

  assert(sock->_.arg.obj == NULL);
  arg.yieldback = CTX(sock)->_.current_coro;
  arg.err = dfk_err_ok;
  sock->_.arg.obj = &arg;
  request.data = sock;

  err = uv_write(
      &request,
      (uv_stream_t*) &sock->_.socket,
      &uvbuf,
      1,
      dfk_tcp_socket_on_write);
  if (err != 0) {
    CTX(sock)->sys_errno = err;
    return dfk_err_sys;
  }

  sock->_.flags |= TCP_SOCKET_WRITING;
  if ((err = dfk_coro_yield_to(CTX(sock), &LOOP(sock)->_.coro)) != dfk_err_ok) {
    return err;
  }

  assert(sock->_.arg.obj == NULL);
  assert(STATE(sock) & TCP_SOCKET_WRITING);
  sock->_.flags ^= TCP_SOCKET_WRITING;

  if (arg.err != dfk_err_ok) {
    return arg.err;
  }

  return dfk_err_ok;
}

int dfk_tcp_socket_writev(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov)
{
  (void) sock;
  (void) iov;
  (void) niov;
  return dfk_err_ok;
}

