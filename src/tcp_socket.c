#include <dfk/tcp_socket.h>

int dfk_tcp_socket_init(dfk_tcp_socket_t* obj, dfk_event_loop_t* loop)
{
  uv_tcp_init(&loop->_.loop, &obj->_.socket);
  return dfk_err_ok;
}

int dfk_tcp_socket_free(dfk_tcp_socket_t* obj)
{
  (void) obj;
  return dfk_err_ok;
}

int dfk_tcp_socket_connect(dfk_tcp_socket_t* obj, const char* endpoint, uint16_t port)
{
  (void) obj;
  (void) endpoint;
  (void) port;
  return dfk_err_ok;
}

int dfk_tcp_socket_close(dfk_tcp_socket_t* obj)
{
  (void) obj;
  return dfk_err_ok;
}

int dfk_tcp_socket_listen(
    dfk_tcp_socket_t* obj,
    const char* endpoint,
    uint16_t port,
    void (*accept)(dfk_tcp_socket_t*),
    size_t backlog)
{
  (void) obj;
  (void) endpoint;
  (void) port;
  (void) accept;
  (void) backlog;
  return dfk_err_ok;
}

int dfk_tcp_socket_read(
    dfk_tcp_socket_t* obj,
    char* buf,
    size_t nbytes,
    size_t* nread)
{
  (void) obj;
  (void) buf;
  (void) nbytes;
  (void) nread;
  return dfk_err_ok;
}

int dfk_tcp_socket_readv(
    dfk_tcp_socket_t* obj,
    dfk_iovec_t* iov,
    size_t niov,
    size_t *nread)
{
  (void) obj;
  (void) iov;
  (void) niov;
  (void) nread;
  return dfk_err_ok;
}

int dfk_tcp_socket_readall(
    dfk_tcp_socket_t* obj,
    size_t nbytes,
    dfk_buf_t* buf)
{
  (void) obj;
  (void) nbytes;
  (void) buf;
  return dfk_err_ok;
}

int dfk_tcp_socket_write(
    dfk_tcp_socket_t* obj,
    char* buf,
    size_t nbytes,
    size_t* nwritten)
{
  (void) obj;
  (void) buf;
  (void) nbytes;
  (void) nwritten;
  return dfk_err_ok;
}

int dfk_tcp_socket_writev(
    dfk_tcp_socket_t* obj,
    dfk_iovec_t* iov,
    size_t niov,
    size_t* nwritten)
{
  (void) obj;
  (void) iov;
  (void) niov;
  (void) nwritten;
  return dfk_err_ok;
}
