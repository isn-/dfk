/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
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

#define STATE(sock) ((sock)->_flags & 0xFF)
#define TO_STATE(sock, state) ((sock)->_flags = ((sock)->_flags & 0xFFFFFF00) | (state))

/* Accepted socket has an accociated coroutine and hence
 * requires explicit coroutine join upon closing
 */
#define TCP_SOCKET_IS_ACCEPTED ((int) 1 << 8)
#define TCP_SOCKET_IS_LISTENING ((int) 1 << 9)
#define TCP_SOCKET_DETACHED ((int) 1 << 10)

#define FLAG(sock, flag) (((sock)->_flags & 0xFFFFFF00) & (flag))


int dfk_tcp_socket_init(dfk_tcp_socket_t* sock, dfk_t* dfk)
{
  if (!sock || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) sock);

  sock->_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (sock->_socket == -1) {
    DFK_ERROR(dfk, "socket() call failed with code %d: %s", errno, strerror(errno));
    dfk->sys_errno = errno;
    return dfk_err_sys;
  }

  sock->_arg.func = NULL;
  sock->_flags = TCP_SOCKET_SPARE;
  sock->dfk = dfk;
  return dfk_err_ok;
}


int dfk_tcp_socket_free(dfk_tcp_socket_t* sock)
{
  if (!sock) {
    return dfk_err_badarg;
  }
  DFK_DBG(sock->dfk, "{%p}", (void*) sock);
  if (STATE(sock) != TCP_SOCKET_CLOSED) {
    DFK_CALL(sock->dfk, dfk_tcp_socket_close(sock));
  }
  return dfk_err_ok;
}


/**
 * Sets connect result and yields back to dfk_tcp_socket_connect caller
 */
static void dfk__tcp_socket_on_connect(
    int status,
    dfk_epoll_cb_arg_t arg0,
    dfk_epoll_cb_arg_t arg1,
    dfk_epoll_cb_arg_t arg2,
    dfk_epoll_cb_arg_t arg3)
{
  dfk_tcp_socket_t* sock = (dfk_tcp_socket_t*) arg0.ptr;
  int* err = (int*) arg1.ptr;
  dfk_coro_t* yieldback = (dfk_coro_t*) arg2.ptr;
  DFK_UNUSED(arg3);

  assert(sock);
  assert(STATE(sock) == TCP_SOCKET_CONNECTING);

  DFK_DBG(sock->dfk, "{%p} connected with status %d (%s)",
      (void*) sock, status, status ? "error" : "no error");

  epoll_ctl(sock->dfk->_epollfd, EPOLL_CTL_DEL, sock->_socket, NULL);

  if (status != 0) {
    sock->dfk->sys_errno = status;
    *err = dfk_err_sys;
  } else {
    *err = dfk_err_ok;
  }
  DFK_IO_RESUME(sock->dfk, yieldback);
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

  struct sockaddr_in dest;
  memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_port = htons(port);
  inet_aton(endpoint, (struct in_addr*) &dest.sin_addr.s_addr);

  if (connect(sock->_socket, &dest, sizeof(dest)) == -1) {
    DFK_ERROR(sock->dfk, "connect() failed with code %d: %s", errno, strerror(errno));
    sock->dfk->sys_errno = errno;
    return dfk_err_sys;
  }

  int err = dfk_err_ok;
  dfk_epoll_arg_t arg = {
    .callback = dfk__tcp_socket_on_connect,
    .arg0 = {.ptr = sock},
    .arg1 = {.ptr = &err},
    .arg2 = {.ptr = DFK_THIS_CORO(sock->dfk)},
    .arg3 = {.ptr = NULL}
  };

  struct epoll_event event;
  event.data.ptr = &arg;
  event.events = EPOLLOUT;
  epoll_ctl(sock->dfk->_epollfd, EPOLL_CTL_ADD, sock->_socket, &event);

  TO_STATE(sock, TCP_SOCKET_CONNECTING);
  DFK_IO(sock->dfk);
  TO_STATE(sock, err = dfk_err_ok ? TCP_SOCKET_CONNECTED : TCP_SOCKET_SPARE);

  return err;
}


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


/**
 * When socket is in LISTENING state, a pointer to
 * dfk_tcp_socket_listen_arg_t is stored in sock->_arg
 */
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


static void dfk__tcp_socket_on_close(uv_handle_t* p)
{
  dfk_tcp_socket_t* sock;

  assert(p);
  sock = (dfk_tcp_socket_t*) p->data;
  assert(sock);
  assert(sock->dfk);
  assert(STATE(sock) & TCP_SOCKET_CLOSING);
  DFK_INFO(sock->dfk, "{%p} is now closed", (void*) sock);
  if (STATE(sock) & TCP_SOCKET_LISTENING) {
    dfk_tcp_socket_listen_obj_t* obj = (dfk_tcp_socket_listen_obj_t*) sock->_arg.data;
    DFK_IO_RESUME(sock->dfk, obj->close_yieldback);
    DFK_IO_RESUME(sock->dfk, obj->yieldback);
  } else {
    DFK_IO_RESUME(sock->dfk, (dfk_coro_t*) sock->_arg.data);
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
  if (STATE(sock) & TCP_SOCKET_LISTENING) {
    ((dfk_tcp_socket_listen_obj_t*) sock->_arg.data)->close_yieldback =
      DFK_THIS_CORO(sock->dfk);
  } else {
    sock->_arg.data = DFK_THIS_CORO(sock->dfk);
  }
  sock->_flags |= TCP_SOCKET_CLOSING;
  DFK_INFO(sock->dfk, "{%p} start close", (void*) sock);
  uv_close((uv_handle_t*) &sock->_socket, dfk__tcp_socket_on_close);
  DFK_IO(sock->dfk);
  DFK_INFO(sock->dfk, "{%p} returned from on_close callback", (void*) sock);
  assert(STATE(sock) & TCP_SOCKET_CLOSING);
  TO_STATE(sock, TCP_SOCKET_CLOSED);
  return dfk_err_ok;
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
    /* do nothing thus far */
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
    /* read returned EAGAIN or EWOULDBLOCK */
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

  if (STATE(sock) & TCP_SOCKET_READING) {
    return dfk_err_inprog;
  }

  if (!(STATE(sock) & TCP_SOCKET_CONNECTED)) {
    return dfk_err_badarg;
  }

  {
    dfk_tcp_socket_read_async_arg_t arg;

    DFK_DBG(sock->dfk, "{%p} read upto %lu bytes to %p",
        (void*) sock, (unsigned long) nbytes, (void*) buf);
    sock->_flags |= TCP_SOCKET_READING;

    assert(sock->_arg.data == NULL);
    arg.yieldback = DFK_THIS_CORO(sock->dfk);
    arg.nbytes = nbytes;
    arg.buf = buf;
    arg.err = dfk_err_ok;
    sock->_arg.data = &arg;

    DFK_SYSCALL(sock->dfk, uv_read_start(
        (uv_stream_t*) &sock->_socket,
        dfk__tcp_socket_on_alloc,
        dfk__tcp_socket_on_read));

    DFK_IO(sock->dfk);

    sock->_arg.data = NULL;
    assert(STATE(sock) & TCP_SOCKET_READING);
    sock->_flags ^= TCP_SOCKET_READING;

    DFK_INFO(sock->dfk, "(%p) read returned %s, bytes read %lu",
        (void*) sock, dfk_strerr(sock->dfk, arg.err),
        (unsigned long) arg.nbytes);

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
  if (!sock || (!iov && niov)) {
    return dfk_err_badarg;
  }
  if (!niov) {
    return dfk_err_ok;
  }
  /* optimizations are not supported yet */
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

