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
#include <dfk.h>
#include <dfk/internal.h>

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
#define TCP_SOCKET_IS_LISTENING ((int) 1 << 9)
#define TCP_SOCKET_DETACHED ((int) 1 << 10)

#define FLAG(sock, flag) (((sock)->_.flags & 0xFFFFFF00) & (flag))


int dfk_tcp_socket_init(dfk_tcp_socket_t* sock, dfk_t* dfk)
{
  if (!sock || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) sock);
  {
    int err = uv_tcp_init(dfk->_.uvloop, &sock->_.socket);
    sock->_.socket.data = sock;
    if (err != 0) {
      DFK_ERROR(dfk, "{%p} uv_tcp_init returned %d", (void*) sock, err);
      dfk->sys_errno = err;
      return dfk_err_sys;
    }
  }
  sock->_.arg.func = NULL;
  sock->_.flags = TCP_SOCKET_SPARE;
  sock->userdata = NULL;
  sock->dfk = dfk;
  return dfk_err_ok;
}


int dfk_tcp_socket_free(dfk_tcp_socket_t* sock)
{
  if (!sock) {
    return dfk_err_badarg;
  }
  DFK_DBG(sock->dfk, "{%p}", (void*) sock);
  if (STATE(sock) != TCP_SOCKET_SPARE) {
    int err;
    if ((err = dfk_tcp_socket_close(sock)) != dfk_err_ok) {
      return err;
    }
  }
  return dfk_err_ok;
}


typedef struct dfk_tcp_socket_connect_async_arg_t {
  int err;
  dfk_coro_t* yieldback;
} dfk_tcp_socket_connect_async_arg_t;


/**
 * Sets connect result and yields back to dfk_tcp_socket_connect caller
 */
static void dfk_tcp_socket_on_connect(uv_connect_t* p, int status)
{
  dfk_tcp_socket_t* sock = (dfk_tcp_socket_t*) p->data;
  dfk_tcp_socket_connect_async_arg_t* arg;

  assert(p);
  assert(sock);
  assert(STATE(sock) == TCP_SOCKET_CONNECTING);

  DFK_DBG(sock->dfk, "{%p} connected with status %d (%s)",
      (void*) sock, status, status ? "error" : "no error");

  arg = (dfk_tcp_socket_connect_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  if (status != 0) {
    sock->dfk->sys_errno = status;
    arg->err = dfk_err_sys;
  } else {
    arg->err = dfk_err_ok;
  }
  {
    int err = dfk_yield(sock->dfk->_.eventloop, arg->yieldback);
    if (err != dfk_err_ok) {
      DFK_ERROR(sock->dfk, "{%p} dfk_yield returned %d", (void*) sock, err);
    }
  }
}


int dfk_tcp_socket_connect(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port)
{
  if (!sock || !endpoint) {
    return dfk_err_badarg;
  }
  if (STATE(sock) == TCP_SOCKET_CONNECTING) {
    return dfk_err_inprog;
  }
  if (STATE(sock) != TCP_SOCKET_SPARE) {
    return dfk_err_badarg;
  }

  DFK_DBG(sock->dfk, "{%p} connect to %s:%d", (void*) sock, endpoint, port);

  {
    struct sockaddr_in dest;
    uv_connect_t connect;
    dfk_tcp_socket_connect_async_arg_t arg;

    DFK_SYSCALL(sock->dfk, uv_ip4_addr(endpoint, port, &dest));

    arg.err = dfk_err_ok;
    arg.yieldback = DFK_THIS_CORO(sock->dfk);
    assert(sock->_.arg.obj == NULL);
    sock->_.arg.obj = &arg;
    connect.data = sock;

    DFK_SYSCALL(sock->dfk, uv_tcp_connect(
        &connect,
        &sock->_.socket,
        (const struct sockaddr*) &dest,
        dfk_tcp_socket_on_connect));

    TO_STATE(sock, TCP_SOCKET_CONNECTING);
    DFK_YIELD_EVENTLOOP(sock->dfk);

    sock->_.arg.obj = NULL;
    if (arg.err == dfk_err_ok) {
      TO_STATE(sock, TCP_SOCKET_CONNECTED);
    } else {
      TO_STATE(sock, TCP_SOCKET_SPARE);
    }
    return arg.err;
  }
}


/*
typedef struct _accepted_socket_t {
  dfk_tcp_socket_t socket;
  dfk_coro_t coro;
} _accepted_socket_t;


typedef struct {
  dfk_tcp_socket_t* sock;
  dfk_tcp_socket_t* lsock;
} _accepted_main_1_arg;


static void dfk_tcp_socket_accepted_main_1(void* varg)
{
  int err;
  void (*callback)(dfk_tcp_socket_t*, dfk_tcp_socket_t*, int);
  _accepted_main_1_arg arg;

  assert(varg);
  arg = *((_accepted_main_1_arg*) varg);
  assert(arg.sock);
  arg.sock->_.flags |= TCP_SOCKET_IS_ACCEPTED;
  TO_STATE(arg.sock, TCP_SOCKET_CONNECTED);
  callback = (void(*)(dfk_tcp_socket_t*, dfk_tcp_socket_t*, int)) arg.sock->_.arg.func;
  assert(callback);
  arg.sock->_.arg.func = NULL;

  DFK_DBG(CTX(arg.sock), "call listen callback with args (%p, %p, %d)",
      (void*) arg.lsock, (void*) arg.sock, 0);
  callback(arg.lsock, arg.sock, dfk_err_ok);
  DFK_DBG(CTX(arg.sock), "listen callback exited");

  if ((err = dfk_tcp_socket_free(arg.sock)) != dfk_err_ok) {
    DFK_ERROR(CTX(arg.sock), "(%p) dfk_socket_free returned %d", (void*) arg.sock, err);
  }
}


typedef struct dfk_tcp_socket_listen_arg_t {
  dfk_coro_t* yieldback;
  dfk_coro_t* close_yieldback;
  void (*callback)(dfk_tcp_socket_t*, dfk_tcp_socket_t*, int);
  int err;
} dfk_tcp_socket_listen_arg_t;


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
  TO_STATE(sock, TCP_SOCKET_CONNECTED);
  callback = (void(*)(dfk_tcp_socket_t*)) sock->_.arg.func;
  assert(callback);

  callback(sock);

  if ((err = dfk_tcp_socket_free(sock)) != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_socket_free returned %d", (void*) sock, err);
  }
}


static void dfk_tcp_socket_on_new_connection_2(uv_stream_t* p, int status)
{
  int err;
  dfk_tcp_socket_t* sock;
  _accepted_socket_t* newsock;
  dfk_tcp_socket_listen_arg_t* arg;

  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (dfk_tcp_socket_listen_arg_t*) sock->_.arg.obj;
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
  err = dfk_coro_init(&newsock->coro, CTX(sock), 0);
  if (err != dfk_err_ok) {
    DFK_FREE(CTX(sock), newsock);
    arg->err = err;
    return;
  }

#ifdef DFK_ENABLE_NAMED_COROUTINES
  snprintf(newsock->coro.name, sizeof(newsock->coro.name),
      "socket.%p.main", (void*) newsock);
#endif

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
    void (*callback)(dfk_tcp_socket_t*, dfk_tcp_socket_t*, int),
    size_t backlog)
{
  int err;
  struct sockaddr_in bind;
  dfk_tcp_socket_listen_arg_t arg;

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
  arg.close_yieldback = NULL;
  arg.err = dfk_err_ok;
  arg.callback = callback;
  sock->_.arg.obj = &arg;

  if ((err = dfk_coro_yield_to(CTX(sock), &LOOP(sock)->_.coro)) != dfk_err_ok) {
    return err;
  }

  return arg.err;
}

static void dfk_tcp_socket_on_close_regular(dfk_tcp_socket_t* sock, void* varg)
{
  int err;
  dfk_coro_t* callback = (dfk_coro_t*) varg;
  DFK_DBG(CTX(sock), "regular socket closed, yieldback to the caller");
  err = dfk_coro_yield_to(CTX(sock), callback);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
  }
}

static void dfk_tcp_socket_on_close_accepted(dfk_tcp_socket_t* sock, void* varg)
{
  int err;
  if ((err = dfk_coro_join(&((_accepted_socket_t*) sock)->coro)) != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_join returned %d", (void*) sock, err);
  }
  DFK_FREE(CTX(sock), sock);
  (void) varg;
}

typedef struct {
  dfk_context_t* ctx;
  dfk_coro_t* coro;
} _poke_arg_t;

static void poke(void* varg)
{
  int err;
  _poke_arg_t* arg = (_poke_arg_t*) varg;
  if ((err = dfk_coro_yield_to(arg->ctx, arg->coro)) != dfk_err_ok) {
    DFK_ERROR(arg->ctx, "dfk_coro_yield_to returned %d", err);
  }
}

static void dfk_tcp_socket_on_close_listening(dfk_tcp_socket_t* sock, void* varg)
{
  int err;
  dfk_tcp_socket_listen_arg_t* arg = (dfk_tcp_socket_listen_arg_t*) varg;
  dfk_once_t once;
  _poke_arg_t pokearg;
  pokearg.ctx = CTX(sock);
  pokearg.coro = CTX(sock)->_.current_coro;
  dfk_once_init(&once, LOOP(sock), poke, &pokearg);
  dfk_once_fire(&once);
  err = dfk_coro_yield_to(CTX(sock), arg->close_yieldback);
  if (err != dfk_err_ok) {
    DFK_ERROR(CTX(sock), "(%p) dfk_coro_yield_to returned %d", (void*) sock, err);
  }
  dfk_once_free(&once);
  dfk_tcp_socket_on_close_regular(sock, arg->yieldback);
}
*/


static void dfk_tcp_socket_on_close(uv_handle_t* p)
{
  dfk_tcp_socket_t* sock;

  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  assert(sock->dfk);
  assert(STATE(sock) & TCP_SOCKET_CLOSING);
  DFK_INFO(sock->dfk, "{%p} is now closed", (void*) sock);

  {
    dfk_coro_t* arg = (dfk_coro_t*) sock->_.arg.obj;
    int err = dfk_yield(sock->dfk->_.eventloop, arg);
    if (err != dfk_err_ok) {
      DFK_ERROR(sock->dfk, "(%p) dfk_yield returned %d", (void*) sock, err);
    }
  }
}


int dfk_tcp_socket_close(dfk_tcp_socket_t* sock)
{
  if (!sock) {
    return dfk_err_badarg;
  }
  if (STATE(sock) & TCP_SOCKET_CLOSING) {
    return dfk_err_inprog;
  }
  if (STATE(sock) == TCP_SOCKET_CLOSED) {
    return dfk_err_ok;
  }

  assert(sock->dfk);
  assert(sock->_.arg.obj == NULL);
  sock->_.arg.obj = DFK_THIS_CORO(sock->dfk);
  sock->_.flags |= TCP_SOCKET_CLOSING;
  DFK_INFO(sock->dfk, "{%p} start close", (void*) sock);
  uv_close((uv_handle_t*) &sock->_.socket, dfk_tcp_socket_on_close);
  DFK_YIELD_EVENTLOOP(sock->dfk);
  sock->_.arg.obj = NULL;
  assert(STATE(sock) & TCP_SOCKET_CLOSING);
  TO_STATE(sock, TCP_SOCKET_CLOSED);
  return dfk_err_ok;
}


typedef struct dfk_tcp_socket_read_async_arg_t {
  dfk_coro_t* yieldback;
  size_t nbytes;
  char* buf;
  int err;
} dfk_tcp_socket_read_async_arg_t;


static void dfk_tcp_socket_on_read(uv_stream_t* p, ssize_t nread, const uv_buf_t* buf)
{
  dfk_tcp_socket_read_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  DFK_UNUSED(buf);
  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (dfk_tcp_socket_read_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  DFK_INFO(sock->dfk, "(%p) %ld bytes read", (void*) sock, (long) nread);
  if (nread < 0) {
    arg->err = dfk_err_sys;
  } else {
    arg->nbytes = nread;
    arg->err = dfk_err_ok;
  }

  {
    int err = uv_read_stop(p);
    if (err != 0) {
      arg->err = dfk_err_sys;
      sock->dfk->sys_errno = err;
    }
  }

  {
    int err = dfk_yield(sock->dfk->_.eventloop, arg->yieldback);
    if (err != dfk_err_ok) {
      DFK_ERROR(sock->dfk, "(%p) dfk_yield returned %d", (void*) sock, err);
    }
  }
}

static void dfk_tcp_socket_on_alloc(uv_handle_t* p, size_t hint, uv_buf_t* buf)
{
  dfk_tcp_socket_read_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  (void) hint;
  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (dfk_tcp_socket_read_async_arg_t*) sock->_.arg.obj;
  assert(arg);

  DFK_DBG(sock->dfk, "(%p) %lu bytes requested", (void*) sock, (unsigned long) hint);

  if (arg->nbytes) {
    *buf = uv_buf_init(arg->buf, arg->nbytes);
  }
}


ssize_t dfk_tcp_socket_read(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes)
{
  if (!sock || !buf) {
    return dfk_err_badarg;
  }

  if (nbytes == 0) {
    return 0;
  }

  {
    dfk_tcp_socket_read_async_arg_t arg;

    DFK_DBG(sock->dfk, "{%p} read upto %lu bytes to %p",
        (void*) sock, (unsigned long) nbytes, (void*) buf);
    if (STATE(sock) & TCP_SOCKET_READING) {
      return dfk_err_inprog;
    }
    sock->_.flags |= TCP_SOCKET_READING;

    assert(sock->_.arg.obj == NULL);
    arg.yieldback = DFK_THIS_CORO(sock->dfk);
    arg.nbytes = nbytes;
    arg.buf = buf;
    arg.err = dfk_err_ok;
    sock->_.arg.obj = &arg;

    DFK_SYSCALL(sock->dfk, uv_read_start(
        (uv_stream_t*) &sock->_.socket,
        dfk_tcp_socket_on_alloc,
        dfk_tcp_socket_on_read));

    DFK_YIELD_EVENTLOOP(sock->dfk);

    sock->_.arg.obj = NULL;
    assert(STATE(sock) & TCP_SOCKET_READING);
    sock->_.flags ^= TCP_SOCKET_READING;

    DFK_INFO(sock->dfk, "(%p) read returned %d, bytes read %lu",
        (void*) sock, arg.err, (unsigned long) arg.nbytes);

    if (arg.err != dfk_err_ok) {
      sock->dfk->dfk_errno = arg.err;
      return -1;
    }

    return arg.nbytes;
  }
}


ssize_t dfk_tcp_socket_readv(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov)
{
  if (!sock) {
    return -1;
  }
  assert(sock->dfk);
  if (!iov) {
    sock->dfk->dfk_errno = dfk_err_badarg;
    return -1;
  }
  DFK_UNUSED(niov);
  sock->dfk->dfk_errno = dfk_err_not_implemented;
  return -1;
}


typedef struct dfk_tcp_socket_write_async_arg_t {
  dfk_coro_t* yieldback;
  int err;
} dfk_tcp_socket_write_async_arg_t;


static void dfk_tcp_socket_on_write(uv_write_t* request, int status)
{
  dfk_tcp_socket_write_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  assert(request);
  sock = (dfk_tcp_socket_t*) request->data;
  assert(sock);
  arg = (dfk_tcp_socket_write_async_arg_t*) sock->_.arg.obj;
  assert(arg);
  sock->_.arg.obj = NULL;

  DFK_DBG(sock->dfk, "(%p) write returned %d", (void*) sock, status);

  if (status < 0) {
    sock->dfk->sys_errno = status;
    arg->err = dfk_err_sys;
  } else {
    arg->err = dfk_err_ok;
  }

  {
    int err = dfk_yield(sock->dfk->_.eventloop, arg->yieldback);
    if (err != dfk_err_ok) {
      DFK_ERROR(sock->dfk, "(%p) dfk_yield returned %d", (void*) sock, err);
    }
  }
}


ssize_t dfk_tcp_socket_write(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes)
{
  if (!sock) {
    return -1;
  }
  if (!buf && nbytes != 0) {
    sock->dfk->dfk_errno = dfk_err_badarg;
    return -1;
  }

  {
    uv_write_t request;
    dfk_tcp_socket_write_async_arg_t arg;
    uv_buf_t uvbuf = uv_buf_init(buf, nbytes);

    assert(sock->dfk);

    if (STATE(sock) & TCP_SOCKET_WRITING) {
      sock->dfk->dfk_errno = dfk_err_inprog;
      return -1;
    }

    if (!(STATE(sock) & TCP_SOCKET_CONNECTED)) {
      sock->dfk->dfk_errno = dfk_err_badarg;
      return -1;
    }

    DFK_DBG(sock->dfk, "(%p) write %lu bytes",
        (void*) sock, (unsigned long) nbytes);

    if (nbytes == 0) {
      return dfk_err_ok;
    }

    assert(sock->_.arg.obj == NULL);
    arg.yieldback = DFK_THIS_CORO(sock->dfk);
    arg.err = dfk_err_ok;
    sock->_.arg.obj = &arg;
    request.data = sock;

    DFK_SYSCALL(sock->dfk, uv_write(
        &request,
        (uv_stream_t*) &sock->_.socket,
        &uvbuf,
        1,
        dfk_tcp_socket_on_write));

    sock->_.flags |= TCP_SOCKET_WRITING;

    DFK_YIELD_EVENTLOOP(sock->dfk);

    sock->_.arg.obj = NULL;
    assert(STATE(sock) & TCP_SOCKET_WRITING);
    sock->_.flags ^= TCP_SOCKET_WRITING;

    sock->dfk->dfk_errno = arg.err;
    return arg.err == dfk_err_ok ? (ssize_t) nbytes : -1;
  }
}


ssize_t dfk_tcp_socket_writev(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov)
{
  if (!sock) {
    return -1;
  }
  assert(sock->dfk);
  if (!iov) {
    sock->dfk->dfk_errno = dfk_err_badarg;
    return -1;
  }
  DFK_UNUSED(niov);
  sock->dfk->dfk_errno = dfk_err_not_implemented;
  return -1;
}

