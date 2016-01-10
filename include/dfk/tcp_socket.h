#pragma once
#include <uv.h>
#include <dfk/core.h>
#include <dfk/buf.h>
#include <dfk/event_loop.h>

typedef struct {
  struct {
    uv_tcp_t socket;
  } _;
} dfk_tcp_socket_t;

int dfk_tcp_socket_init(dfk_tcp_socket_t* obj, dfk_event_loop_t* loop);
int dfk_tcp_socket_free(dfk_tcp_socket_t* obj);

int dfk_tcp_socket_connect(dfk_tcp_socket_t* obj, const char* endpoint, uint16_t port);
int dfk_tcp_socket_close(dfk_tcp_socket_t* obj);

int dfk_tcp_socket_listen(
    dfk_tcp_socket_t* obj,
    const char* endpoint,
    uint16_t port,
    void (*accept)(dfk_tcp_socket_t*),
    size_t backlog);

int dfk_tcp_socket_read(
    dfk_tcp_socket_t* obj,
    char* buf,
    size_t nbytes,
    size_t* nread);

int dfk_tcp_socket_readv(
    dfk_tcp_socket_t* obj,
    dfk_iovec_t* iov,
    size_t niov,
    size_t *nread);

int dfk_tcp_socket_readall(
    dfk_tcp_socket_t* obj,
    size_t nbytes,
    dfk_buf_t* buf);

int dfk_tcp_socket_write(
    dfk_tcp_socket_t* obj,
    char* buf,
    size_t nbytes,
    size_t* nwritten);

int dfk_tcp_socket_writev(
    dfk_tcp_socket_t* obj,
    dfk_iovec_t* iov,
    size_t niov,
    size_t* nwritten);
