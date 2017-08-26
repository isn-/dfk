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
#include <dfk/make_nonblock.h>
#include <dfk/read.h>
#include <dfk/close.h>

#if DFK_DEBUG
static const char* dfk__str_shutdown_type(dfk_shutdown_type how)
{
  switch(how) {
    case DFK_SHUT_RD:
      return "RD";
    case DFK_SHUT_WR:
      return "WR";
    case DFK_SHUT_RDWR:
      return "RDWR";
    default:
      assert(0 && "Unexpected dfk_shutdown_type value");
  }
  return "Unknown";
}
#endif

int dfk_tcp_socket_init(dfk_tcp_socket_t* sock, dfk_t* dfk)
{
  assert(sock);
  assert(dfk);
  DFK_DBG(dfk, "{%p}", (void*) sock);
#if DFK_HAVE_SOCK_NONBLOCK
  int s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
#else
  int s = socket(AF_INET, SOCK_STREAM, 0);
#endif
  if (s == -1) {
    DFK_ERROR_SYSCALL(dfk, "socket");
    return dfk_err_sys;
  }
#if !DFK_HAVE_SOCK_NONBLOCK
  {
    int err = dfk__make_nonblock(dfk, s);
    if (err != dfk_err_ok) {
      dfk__close(dfk, NULL, s);
      return err;
    }
  }
#endif
  sock->_socket = s;
  sock->dfk = dfk;
  return dfk_err_ok;
}

int dfk_tcp_socket_close(dfk_tcp_socket_t* sock)
{
  assert(sock);
  assert(sock->dfk);
  DFK_DBG(sock->dfk, "{%p}", (void*) sock);
  return dfk__close(sock->dfk, sock, sock->_socket);
}

int dfk_tcp_socket_connect(dfk_tcp_socket_t* sock,
    const char* endpoint, uint16_t port)
{
  assert(sock);
  assert(endpoint);
  assert(sock->dfk);

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
      int ioret = DFK_IO(dfk, sock->_socket, DFK_IO_OUT);
#if DFK_DEBUG
      char strev[16];
      size_t nwritten = dfk__io_events_to_str(ioret, strev, DFK_SIZE(strev));
      DFK_DBG(dfk, "{%p} DFK_IO returned %d (%.*s)", (void*) sock, ioret,
          (int) nwritten, strev);
#endif
      if (ioret & DFK_IO_ERR) {
        DFK_ERROR_SYSCALL(dfk, "connect");
        return dfk_err_sys;
      }
      int err;
      socklen_t errlen = sizeof(err);
      ret = getsockopt(sock->_socket, SOL_SOCKET, SO_ERROR, &err, &errlen);
      if (ret == -1) {
        DFK_ERROR_SYSCALL(dfk, "getsockopt");
        return dfk_err_sys;
      }
      if (err != 0) {
        DFK_ERROR(dfk, "{%p} connect failed, errno=%d: %s",
            (void*) dfk, err, strerror(err));
        if (err == ECONNREFUSED) {
          return dfk_err_connection_refused;
        }
        dfk->sys_errno = err;
        return dfk_err_sys;
      }
    } else if (errno == ECONNREFUSED) {
      return dfk_err_connection_refused;
    }
  } else {
    DFK_DBG(dfk, "{%p} connect returned OK without blocking", (void*) sock);
  }
  DFK_DBG(dfk, "{%p} connected", (void*) sock);
  return dfk_err_ok;
}

ssize_t dfk_tcp_socket_read(dfk_tcp_socket_t* sock, char* buf, size_t nbytes)
{
  assert(sock);
  assert(buf);
  assert(nbytes);
  assert(sock->dfk);
  return dfk__read(sock->dfk, sock, sock->_socket, buf, nbytes);
}

ssize_t dfk_tcp_socket_write(dfk_tcp_socket_t* sock, char* buf, size_t nbytes)
{
  assert(sock);
  assert(buf);
  assert(nbytes);
  assert(sock->dfk);

  dfk_t* dfk = sock->dfk;

  ssize_t nwritten = write(sock->_socket, buf, nbytes);
  DFK_DBG(dfk, "{%p} write (possibly blocking) attempt returned %lld, errno=%d",
      (void*) sock, (long long) nwritten, errno);
  if (nwritten >= 0) {
    DFK_DBG(dfk, "{%p} non-blocking write of %lld bytes", (void*) sock,
        (long long) nwritten);
    return nwritten;
  }
  if (errno != EAGAIN) {
    DFK_ERROR_SYSCALL(dfk, "write");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  int ioret = DFK_IO(dfk, sock->_socket, DFK_IO_OUT);
#if DFK_DEBUG
  char strev[16];
  size_t strevlen = dfk__io_events_to_str(ioret, strev, DFK_SIZE(strev));
  DFK_DBG(dfk, "{%p} DFK_IO returned %d (%.*s)", (void*) sock, ioret,
      (int) strevlen, strev);
#endif
  if (ioret & DFK_IO_ERR) {
    DFK_ERROR_SYSCALL(dfk, "write");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  assert(ioret & DFK_IO_OUT);
  nwritten = write(sock->_socket, buf, nbytes);
  DFK_DBG(dfk, "{%p} write returned %lld", (void*) sock, (long long) nwritten);
  if (nwritten < 0) {
    DFK_ERROR_SYSCALL(dfk, "write");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  return nwritten;
}

typedef struct dfk_tcp_socket_accepted_main_arg_t {
  dfk_tcp_socket_t socket;
  void (*callback)(dfk_fiber_t*, dfk_tcp_socket_t*, dfk_userdata_t);
  dfk_userdata_t callback_ud;
} dfk_tcp_socket_accepted_main_arg_t;

static void dfk__tcp_socket_accepted_main(dfk_fiber_t* fiber, void* arg)
{
  dfk_tcp_socket_accepted_main_arg_t* a = arg;
  assert(fiber);
  assert(a);
  assert(a->callback);
  a->callback(fiber, &a->socket, a->callback_ud);
}

int dfk_tcp_socket_listen(dfk_tcp_socket_t* sock,
    const char* endpoint, uint16_t port,
    void (*callback)(dfk_fiber_t*, dfk_tcp_socket_t*, dfk_userdata_t),
    dfk_userdata_t callback_ud, size_t backlog)
{
  assert(sock);
  assert(endpoint);
  assert(port);
  assert(callback);
  assert(backlog);

  dfk_t* dfk = sock->dfk;

  DFK_DBG(sock->dfk, "{%p} listen address %s:%u, backlog %lu",
      (void*) sock, endpoint, (unsigned int) port, (unsigned long) backlog);

  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  inet_aton(endpoint, (struct in_addr*) &bindaddr.sin_addr.s_addr);
  bindaddr.sin_port = htons(port);

  socklen_t sockaddr_size = sizeof(struct sockaddr_in);

  int err = bind(sock->_socket, (struct sockaddr*) &bindaddr, sockaddr_size);
  if (err < 0) {
    DFK_ERROR_SYSCALL(dfk, "bind");
    return dfk_err_sys;
  }

  err = listen(sock->_socket, backlog);
  if (err < 0) {
    DFK_ERROR_SYSCALL(dfk, "listen");
    return dfk_err_sys;
  }

  while (1) {
    struct sockaddr_in client;
    int s = accept(sock->_socket, (struct sockaddr*) &client, &sockaddr_size);
    if (s < 0) {
      if (errno == EWOULDBLOCK) {
        int ioret = DFK_IO(dfk, sock->_socket, DFK_IO_IN);
#if DFK_DEBUG
        char strev[16];
        size_t nwritten = dfk__io_events_to_str(ioret, strev, DFK_SIZE(strev));
        DFK_DBG(dfk, "{%p} DFK_IO returned %d (%.*s)", (void*) sock, ioret,
            (int) nwritten, strev);
#endif
        if (ioret & DFK_IO_ERR) {
          DFK_ERROR_SYSCALL(dfk, "accept");
          return dfk_err_sys;
        }
      } else {
        DFK_ERROR_SYSCALL(dfk, "accept");
        return dfk_err_sys;
      }
      continue;
    }
    DFK_DBG(dfk, "switch socket %d to non-blocking mode", s);
    int err = dfk__make_nonblock(dfk, s);
    if (err != dfk_err_ok) {
      dfk__close(dfk, NULL, s);
      continue;
    }
    dfk_tcp_socket_accepted_main_arg_t arg = {
      .socket = {
        .dfk = dfk,
       ._socket = s
      },
      .callback = callback,
      .callback_ud = callback_ud
    };
    dfk_run(dfk, dfk__tcp_socket_accepted_main, &arg, sizeof(arg));
  }
}

int dfk_tcp_socket_shutdown(dfk_tcp_socket_t* sock, dfk_shutdown_type how)
{
  assert(sock);
  dfk_t* dfk = sock->dfk;
  DFK_DBG(sock->dfk, "{%p} %s", (void*) sock, dfk__str_shutdown_type(how));
  int native_how = 0;
  switch(how) {
    case DFK_SHUT_RD:
      native_how = SHUT_RD;
      break;
    case DFK_SHUT_WR:
      native_how = SHUT_WR;
      break;
    case DFK_SHUT_RDWR:
      native_how = SHUT_RDWR;
      break;
    default:
      assert(0 && "Unexpected dfk_shutdown_type value");
  }
  int err = shutdown(sock->_socket, native_how);
  if (err) {
    DFK_ERROR_SYSCALL(dfk, "shutdown");
    return dfk_err_sys;
  }
  return dfk_err_ok;
}

