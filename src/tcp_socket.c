/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#define _BSD_SOURCE
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dfk/tcp_socket.h>
#include <dfk/error.h>
#include <dfk/internal.h>

int dfk_tcp_socket_init(dfk_tcp_socket_t* sock, dfk_t* dfk)
{
  assert(sock);
  assert(dfk);
  DFK_DBG(dfk, "{%p}", (void*) sock);
  int s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (s == -1) {
    DFK_ERROR_SYSCALL(dfk, "socket");
    return dfk_err_sys;
  }
  sock->_socket = s;
  sock->dfk = dfk;
  return dfk_err_ok;
}

int dfk_tcp_socket_close(dfk_tcp_socket_t* sock)
{
  assert(sock);
  assert(sock->dfk);
  dfk_t* dfk = sock->dfk;
  DFK_INFO(dfk, "{%p} closing", (void*) sock);
  if (close(sock->_socket) == -1) {
    DFK_ERROR_SYSCALL(dfk, "close");
    return dfk_err_sys;
  }
  return dfk_err_ok;
}

int dfk_tcp_socket_connect(dfk_tcp_socket_t* sock,
    const char* endpoint, uint16_t port)
{
  assert(sock);
  assert(endpoint);
  dfk_t* dfk = sock->dfk;

  DFK_DBG(dfk, "{%p} connect to %s:%d", (void*) sock, endpoint, port);

  struct sockaddr_in dest;
  memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_port = htons(port);
  inet_aton(endpoint, (struct in_addr*) &dest.sin_addr.s_addr);
  int ret = connect(sock->_socket, (struct sockaddr *) &dest, sizeof(dest));
  if (ret == -1) {
    DFK_DBG(dfk, "{%p} connect returned -1, errno=%d: %s",
        (void*) sock, errno, strerror(errno));
    if (errno == EINPROGRESS) {
      int ioret = DFK_IO(dfk, sock->_socket, DFK_IO_IN | DFK_IO_OUT);
      DFK_DBG(dfk, "{%p} DFK_IO returned %d", (void*) sock, ioret);
      if (ioret & DFK_IO_ERR) {
        DFK_ERROR_SYSCALL(dfk, "connect)");
        return dfk_err_sys;
      }
    }
  }
  return dfk_err_ok;
}

ssize_t dfk_tcp_socket_read(dfk_tcp_socket_t* sock, char* buf, size_t nbytes)
{
  assert(sock);
  assert(buf);
  assert(nbytes);
  dfk_t* dfk = sock->dfk;

  ssize_t nread = read(sock->_socket, buf, nbytes);
  DFK_DBG(dfk, "{%p} first read (possibly blocking) attempt returned %lld",
      (void*) sock, (long long) nread);
  if (nread >= 0) {
    return nread;
  }
  if (nread != EAGAIN) {
    DFK_ERROR_SYSCALL(dfk, "read");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  int ioret = DFK_IO(dfk, sock->_socket, DFK_IO_IN);
  DFK_DBG(dfk, "{%p} DFK_IO returned %d", (void*) sock, ioret);
  if (ioret & DFK_IO_ERR) {
    DFK_ERROR_SYSCALL(dfk, "read");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  assert(ioret & DFK_IO_IN);
  nread = read(sock->_socket, buf, nbytes);
  DFK_DBG(dfk, "{%p} read returned %lld", (void*) sock, (long long) nread);
  if (nread < 0) {
    DFK_ERROR_SYSCALL(dfk, "read");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  return nread;
}

/*
typedef struct dfk_tcp_socket_accepted_main_arg_t {
  uv_stream_t* stream;
  void (*callback)(dfk_coro_t*, dfk_tcp_socket_t*, void*);
  void* cbarg;
} dfk_tcp_socket_accepted_main_arg_t;


static void dfk__tcp_socket_accepted_main(dfk_coro_t* coro, void* p)
{
  dfk_tcp_socket_accepted_main_arg_t* arg =
    (dfk_tcp_socket_accepted_main_arg_t*) p;
  dfk_tcp_socket_t sock;

  assert(arg);
  assert(arg->callback);
  assert(arg->stream);

  DFK_CALL_RVOID(coro->dfk, dfk_tcp_socket_init(&sock, coro->dfk));
  DFK_SYSCALL_RVOID(coro->dfk, uv_accept(arg->stream, (uv_stream_t*) &sock._socket));
  TO_STATE(&sock, TCP_SOCKET_CONNECTED);
  DFK_INFO(coro->dfk, "{%p} connection accepted", (void*) &sock);

  arg->callback(coro, &sock, arg->cbarg);
}


 * When socket is in LISTENING state, a pointer to
 * dfk_tcp_socket_listen_arg_t is stored in sock->_arg
typedef struct dfk_tcp_socket_listen_obj_t {
  dfk_coro_t* yieldback;
  dfk_coro_t* close_yieldback;
  void (*callback)(dfk_coro_t*, dfk_tcp_socket_t*, void*);
  void* cbarg;
  int err;
} dfk_tcp_socket_listen_obj_t;


static void dfk__tcp_socket_on_new_connection(uv_stream_t* p, int status)
{
  dfk_tcp_socket_t* sock;
  dfk_tcp_socket_listen_obj_t* arg;

  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (dfk_tcp_socket_listen_obj_t*) sock->_arg.data;
  assert(arg);

  if (status != 0) {
    DFK_ERROR(sock->dfk, "{%p} new connection returned %d", (void*) sock, status);
    sock->dfk->sys_errno = status;
    arg->err = dfk_err_sys;
    return;
  }

  DFK_INFO(sock->dfk, "{%p}", (void*) sock);

  {
    dfk_tcp_socket_accepted_main_arg_t aarg;
    dfk_coro_t* coro;
    aarg.stream = p;
    aarg.callback = arg->callback;
    aarg.cbarg = arg->cbarg;
    coro = dfk_run(sock->dfk, dfk__tcp_socket_accepted_main, &aarg, sizeof(aarg));
    dfk_coro_name(coro, "conn.%p", p->accepted_fd);
  }
}


int dfk_tcp_socket_listen(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_coro_t*, dfk_tcp_socket_t*, void*),
    void* cbarg,
    size_t backlog)
{
  if (sock == NULL || endpoint == NULL || callback == NULL) {
    return dfk_err_badarg;
  }

  if (STATE(sock) != TCP_SOCKET_SPARE) {
    return dfk_err_badarg;
  }

  {
    struct sockaddr_in bind;
    dfk_tcp_socket_listen_obj_t arg;

    DFK_SYSCALL(sock->dfk, uv_ip4_addr(endpoint, port, &bind));
    DFK_SYSCALL(sock->dfk, uv_tcp_bind(&sock->_socket, (struct sockaddr*) &bind, 0));

    if (backlog == 0) {
      backlog = DFK_TCP_BACKLOG;
    }

    assert(!sock->_arg.data);

    DFK_SYSCALL(sock->dfk, uv_listen(
      (uv_stream_t*) &sock->_socket,
      backlog,
      dfk__tcp_socket_on_new_connection));

    TO_STATE(sock, TCP_SOCKET_LISTENING);
    DFK_INFO(sock->dfk, "{%p} listen on %s:%u, backlog %lu",
      (void*) sock, endpoint, port, (unsigned long) backlog);

    arg.yieldback = DFK_THIS_CORO(sock->dfk);
    arg.close_yieldback = NULL;
    arg.callback = callback;
    arg.cbarg = cbarg;
    arg.err = dfk_err_ok;
    sock->_arg.data = &arg;

    DFK_IO(sock->dfk);
    return arg.err;
  }
}


typedef struct dfk__tcp_socket_shutdown_data {
  dfk_coro_t* coro;
  dfk_tcp_socket_t* sock;
  int sys_errno;
} dfk__tcp_socket_shutdown_data;


static void dfk__tcp_socket_on_shutdown(uv_shutdown_t* req, int status)
{
  assert(req);
  assert(req->data);
  dfk__tcp_socket_shutdown_data* sd =
    (dfk__tcp_socket_shutdown_data*) req->data;
  assert(sd->coro);
  DFK_DBG(sd->coro->dfk, "{%p}", (void*) sd->sock);
  sd->sys_errno = status;
  DFK_IO_RESUME(sd->coro->dfk, sd->coro);
}


int dfk_tcp_socket_shutdown(dfk_tcp_socket_t* sock)
{
  if (!sock) {
    return dfk_err_badarg;
  }
  DFK_DBG(sock->dfk, "{%p}", (void*) sock);
  uv_shutdown_t shutdown_req;
  dfk__tcp_socket_shutdown_data sd = {
    .coro = DFK_THIS_CORO(sock->dfk),
    .sock = sock,
    .sys_errno = 0
  };
  shutdown_req.data = &sd;
  uv_shutdown(&shutdown_req, (uv_stream_t*) &sock->_socket, dfk__tcp_socket_on_shutdown);
  DFK_IO(sock->dfk);
  DFK_DBG(sock->dfk, "{%p} shutdown returned %d", (void*) sock, sd.sys_errno);
  if (sd.sys_errno) {
    sock->dfk->sys_errno = sd.sys_errno;
    return dfk_err_sys;
  }
  return dfk_err_ok;
}


typedef struct dfk_tcp_socket_wait_disconnect_data {
  dfk_coro_t* coro;
  dfk_tcp_socket_t* sock;
  int disconnected;
  int sys_errno;
} dfk_tcp_socket_wait_disconnect_data;


static void dfk__tcp_socket_wait_disconnect_expired(uv_timer_t* timer)
{
  assert(timer);
  assert(timer->data);
  dfk_tcp_socket_wait_disconnect_data* pdata =
    (dfk_tcp_socket_wait_disconnect_data*) timer->data;
  DFK_IO_RESUME(pdata->coro->dfk, pdata->coro);
}


static void dfk__tcp_socket_wait_disconnect_callback(uv_poll_t* poll, int status, int events)
{
  assert(poll);
  assert(poll->data);
  dfk_tcp_socket_wait_disconnect_data *wddata =
    (dfk_tcp_socket_wait_disconnect_data*) poll->data;
  DFK_DBG(wddata->coro->dfk, "{%p} events: %d (%s|%s|%s)", (void*) wddata->sock, events,
      (events & UV_DISCONNECT) ? "UV_DISCONNECT " : "",
      (events & UV_READABLE) ? " UV_READABLE " : "",
      (events & UV_WRITABLE) ? " UV_WRITABLE" : "");
  if (status < 0) {
    wddata->sys_errno = status;
    DFK_IO_RESUME(wddata->coro->dfk, wddata->coro);
  } else if (events & UV_DISCONNECT) {
    wddata->disconnected = 1;
    DFK_DBG(wddata->coro->dfk, "{%p} client disconnected, resume", (void*) wddata->sock);
    DFK_IO_RESUME(wddata->coro->dfk, wddata->coro);
  } else if (events & (UV_READABLE | UV_WRITABLE)) {
     * do nothing thus far
  }
}


typedef struct dfk_tcp_socket_wait_disconnect_close_data {
  dfk_coro_t* coro;
  dfk_tcp_socket_t* sock;
  int nclosed;
} dfk_tcp_socket_wait_disconnect_close_data;


static void dfk__tcp_socket_wait_disconnect_on_close(uv_handle_t* handle)
{
  assert(handle);
  dfk_tcp_socket_wait_disconnect_close_data* cd =
    (dfk_tcp_socket_wait_disconnect_close_data*) handle->data;
  DFK_DBG(cd->coro->dfk, "{%p} nclosed:%d", (void*) cd->sock, cd->nclosed);
  if ((++cd->nclosed) == 2) {
    DFK_IO_RESUME(cd->coro->dfk, cd->coro);
  }
}


int dfk_tcp_socket_wait_disconnect(dfk_tcp_socket_t* sock, uint64_t timeout)
{
  if (!sock) {
    return dfk_err_badarg;
  }
  if (!timeout) {
    DFK_POSTPONE(sock->dfk);
    return dfk_err_ok;
  }
  DFK_DBG(sock->dfk, "{%p}", (void*) sock);
  dfk_tcp_socket_wait_disconnect_data wddata = {
    .coro = DFK_THIS_CORO(sock->dfk),
    .sock = sock,
    .disconnected = 0,
    .sys_errno = 0
  };
  uv_timer_t timer;
  uv_timer_init(sock->dfk->_uvloop, &timer);
  timer.data = &wddata;
  uv_timer_start(&timer, dfk__tcp_socket_wait_disconnect_expired, timeout, 0);
  uv_poll_t poll;
  uv_os_fd_t fd;
  uv_fileno((uv_handle_t*) &sock->_socket, &fd);
  uv_poll_init(sock->dfk->_uvloop, &poll, fd);
  poll.data = &wddata;
  uv_poll_start(&poll, UV_DISCONNECT, dfk__tcp_socket_wait_disconnect_callback);
  DFK_IO(sock->dfk);
  DFK_DBG(sock->dfk, "{%p} returned, disconnected:%d", (void*) sock, wddata.disconnected);
  uv_poll_stop(&poll);
  uv_timer_stop(&timer);
  dfk_tcp_socket_wait_disconnect_close_data cd = {
    .coro = DFK_THIS_CORO(sock->dfk),
    .sock = sock
  };
  poll.data = &cd;
  timer.data = &cd;
  uv_close((uv_handle_t*) &poll, dfk__tcp_socket_wait_disconnect_on_close);
  uv_close((uv_handle_t*) &timer, dfk__tcp_socket_wait_disconnect_on_close);
  DFK_DBG(sock->dfk, "{%p} close poll and timer", (void*) sock);
  DFK_IO(sock->dfk);
  if (wddata.sys_errno) {
    sock->dfk->sys_errno = wddata.sys_errno;
    return dfk_err_sys;
  }
  if (wddata.disconnected) {
    return dfk_err_ok;
  } else {
    return dfk_err_timeout;
  }
}


typedef struct dfk_tcp_socket_read_async_arg_t {
  dfk_coro_t* yieldback;
  size_t nbytes;
  char* buf;
  int err;
} dfk_tcp_socket_read_async_arg_t;


static void dfk__tcp_socket_on_read(uv_stream_t* p, ssize_t nread, const uv_buf_t* buf)
{
  dfk_tcp_socket_read_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  DFK_UNUSED(buf);
  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (dfk_tcp_socket_read_async_arg_t*) sock->_arg.data;
  assert(arg);
  DFK_INFO(sock->dfk, "(%p) %ld bytes read", (void*) sock, (long) nread);
  if (nread == 0) {
    * read returned EAGAIN or EWOULDBLOCK
    return;
  } else if (nread < 0) {
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
  DFK_IO_RESUME(sock->dfk, arg->yieldback);
}

static void dfk__tcp_socket_on_alloc(uv_handle_t* p, size_t hint, uv_buf_t* buf)
{
  dfk_tcp_socket_read_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  (void) hint;
  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  arg = (dfk_tcp_socket_read_async_arg_t*) sock->_arg.data;
  assert(arg);

  DFK_DBG(sock->dfk, "(%p) %lu bytes requested", (void*) sock, (unsigned long) hint);

  if (arg->nbytes) {
    *buf = uv_buf_init(arg->buf, arg->nbytes);
  }
}


ssize_t dfk_tcp_socket_readv(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov)
{
  if (!sock || (!iov && niov)) {
    return dfk_err_badarg;
  }
  if (!niov) {
    return dfk_err_ok;
  }
  * optimizations are not supported yet
  return dfk_tcp_socket_read(sock, iov[0].data, iov[0].size);
}


typedef struct dfk_tcp_socket_write_async_arg_t {
  dfk_coro_t* yieldback;
  int err;
} dfk_tcp_socket_write_async_arg_t;


static void dfk__tcp_socket_on_write(uv_write_t* request, int status)
{
  dfk_tcp_socket_write_async_arg_t* arg;
  dfk_tcp_socket_t* sock;

  assert(request);
  sock = (dfk_tcp_socket_t*) request->data;
  assert(sock);
  arg = (dfk_tcp_socket_write_async_arg_t*) sock->_arg.data;
  assert(arg);
  sock->_arg.data = NULL;

  DFK_DBG(sock->dfk, "(%p) write returned %d", (void*) sock, status);

  if (status < 0) {
    sock->dfk->sys_errno = status;
    arg->err = dfk_err_sys;
  } else {
    arg->err = dfk_err_ok;
  }

  DFK_IO_RESUME(sock->dfk, arg->yieldback);
}


ssize_t dfk_tcp_socket_write(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes)
{
  if (!sock || (!buf && nbytes)) {
    return dfk_err_badarg;
  }
  {
    dfk_iovec_t iovec;
    iovec.data = buf;
    iovec.size = nbytes;
    return dfk_tcp_socket_writev(sock, &iovec, 1);
  }
}


ssize_t dfk_tcp_socket_writev(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov)
{
  if (!sock || (!iov && niov)) {
    return dfk_err_badarg;
  }

  if (STATE(sock) & TCP_SOCKET_WRITING) {
    return dfk_err_inprog;
  }

  if (!(STATE(sock) & TCP_SOCKET_CONNECTED)) {
    return dfk_err_badarg;
  }

  {
    uv_write_t request;
    dfk_tcp_socket_write_async_arg_t arg;
    size_t i, totalbytes = 0;

    assert(sock->dfk);

    for (i = 0; i < niov; ++i) {
      totalbytes += iov[i].size;
    }

    DFK_DBG(sock->dfk, "(%p) write %lu bytes from %lu chunks",
        (void*) sock, (unsigned long) totalbytes, (unsigned long) niov);

    if (niov == 0) {
      return dfk_err_ok;
    }

    assert(sock->_arg.data == NULL);
    arg.yieldback = DFK_THIS_CORO(sock->dfk);
    arg.err = dfk_err_ok;
    sock->_arg.data = &arg;
    request.data = sock;

    DFK_SYSCALL(sock->dfk, uv_write(
        &request,
        (uv_stream_t*) &sock->_socket,
        (uv_buf_t*) iov,
        niov,
        dfk__tcp_socket_on_write));

    sock->_flags |= TCP_SOCKET_WRITING;

    DFK_IO(sock->dfk);

    sock->_arg.data = NULL;
    assert(STATE(sock) & TCP_SOCKET_WRITING);
    sock->_flags ^= TCP_SOCKET_WRITING;

    sock->dfk->dfk_errno = arg.err;
    return arg.err == dfk_err_ok ? (ssize_t) totalbytes : -1;
  }
}

*/
