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
#include <dfk/fiber.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A method how to shutdown TCP connection
 *
 * @todo Implement compile-time guessing of enum values to get rid
 * of dfk-to-native constants convertion in the `dfk_tcp_socket_shutdown'.
 */
typedef enum {
  /** Disables further receive operations */
  DFK_SHUT_RD = 1,
  /** Disables further send operations */
  DFK_SHUT_WR = 2,
  /** Disables further send and receive operations */
  DFK_SHUT_RDWR = 3
} dfk_shutdown_type;

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
 *
 * @internal
 * TCP socket object is manually initialized bypassing dfk_tcp_socket_init
 * method when a new incoming connection is accepted. When adding any new code
 * to the dfk_tcp_socket_init method and/or new members to the dfk_tcp_socket_t
 * structure don't forget to handle them in the dfk_tcp_socket_listen.
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
 */
int dfk_tcp_socket_listen(dfk_tcp_socket_t* sock,
    const char* endpoint, uint16_t port,
    void (*callback)(dfk_fiber_t*, dfk_tcp_socket_t*, dfk_userdata_t),
    dfk_userdata_t callback_ud, size_t backlog);

/**
 * Close part of the socket.
 *
 * @see https://linux.die.net/man/3/shutdown
 */
int dfk_tcp_socket_shutdown(dfk_tcp_socket_t* sock, dfk_shutdown_type how);

/**
 * Read data from the socket into several buffers at once
 *
 * @todo Provide optimal implementation
 */
ssize_t dfk_tcp_socket_readv(dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov, size_t niov);

/**
 * Write data from several buffers to socket at once
 *
 * @todo Provide optimal implementation
 */
ssize_t dfk_tcp_socket_writev(dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov, size_t niov);

#ifdef __cplusplus
}
#endif

