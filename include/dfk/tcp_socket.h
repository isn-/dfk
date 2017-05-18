/**
 * @file dfk/tcp_socket.h
 * Contains a definitions of dfk_tcp_socket_t and related routines.
 *
 * @copyright
 * Copyright (c) 2015-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stdint.h>
#include <unistd.h>
#include <dfk/context.h>
#include <dfk/misc.h>

/**
 * TCP socket object
 */
typedef struct dfk_tcp_socket_t {
  /**
   * @publicsection
   */
  dfk_t* dfk;
  dfk_userdata_t user;

  /** @private */
  int _socket;
} dfk_tcp_socket_t;


/**
 * Initialize TCP socket object
 *
 * @return
 * - ::dfk_err_ok On success
 * - ::dfk_err_sys If socket(2) syscall failed
 */
int dfk_tcp_socket_init(dfk_tcp_socket_t* sock, dfk_t* dfk);

/**
 * Close socket.
 *
 * Any operation currently running will be interrupted. Resources allocated to
 * the socket will be released.
 */
int dfk_tcp_socket_close(dfk_tcp_socket_t* sock);

/**
 * Connect to TCP endpoint
 */
int dfk_tcp_socket_connect(dfk_tcp_socket_t* sock,
    const char* endpoint, uint16_t port);

/**
 * Read at most nbytes from the socket
 */
ssize_t dfk_tcp_socket_read(dfk_tcp_socket_t* sock, char* buf, size_t nbytes);

/**
 * Write data to socket
 */
ssize_t dfk_tcp_socket_write(dfk_tcp_socket_t* sock, char* buf, size_t nbytes);


/**
 * Listen for connections on endpoint:port
 *
 * To stop listening, call dfk_tcp_socket_close.
 * Callback is executed for each incoming connection.
int dfk_tcp_socket_listen(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_coro_t*, dfk_tcp_socket_t*, void*),
    void* cbarg,
    size_t backlog);
 */


/**
 * Close the outgoing part of the socket.
int dfk_tcp_socket_shutdown(dfk_tcp_socket_t* sock);
 */


/**
 * Wait for other side of the socket to close connection.
int dfk_tcp_socket_wait_disconnect(dfk_tcp_socket_t* sock, uint64_t timeout);
 */


/**
 * Read data from the socket into several buffers at once
ssize_t dfk_tcp_socket_readv(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov);
 */


/**
 * Write data from several buffers to socket at once
ssize_t dfk_tcp_socket_writev(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov);
 */

