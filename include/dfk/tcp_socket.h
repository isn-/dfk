/**
 * @copyright
 * Copyright (c) 2015-2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#include <stdint.h>
#include <sys/types.h>
#include <dfk/core.h>


/**
 * @file dfk/tcp_socket.h
 * TCP socket object and related functions
 *
 * @addtogroup net net
 * @{
 */

/**
 * TCP socket object
 */
typedef struct dfk_tcp_socket_t {
  /**
   * @privatesection
   */

  int _socket;
  int32_t _flags;

  /**
   * @publicsection
   */

  dfk_t* dfk;
  dfk_userdata_t user;
} dfk_tcp_socket_t;


/**
 * Initialize tcp_socket object
 */
int dfk_tcp_socket_init(dfk_tcp_socket_t* sock, dfk_t* dfk);


/**
 * Cleaup resources allocated for tcp_socket object
 */
int dfk_tcp_socket_free(dfk_tcp_socket_t* sock);


/**
 * Connect to TCP endpoint
 */
int dfk_tcp_socket_connect(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port);


/**
 * Listen for connections on endpoint:port
 *
 * To stop listening, call dfk_tcp_socket_close.
 * Callback is executed for each incoming connection.
 */
int dfk_tcp_socket_listen(
    dfk_tcp_socket_t* sock,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_coro_t*, dfk_tcp_socket_t*, void*),
    void* cbarg,
    size_t backlog);


/**
 * Close socket. Any operation currently running will be interrupted
 */
int dfk_tcp_socket_close(dfk_tcp_socket_t* sock);


/**
 * Close the outgoing part of the socket.
 */
int dfk_tcp_socket_shutdown(dfk_tcp_socket_t* sock);


/**
 * Wait for other side of the socket to close connection.
 */
int dfk_tcp_socket_wait_disconnect(dfk_tcp_socket_t* sock, uint64_t timeout);


/**
 * Read at most nbytes from the socket
 */
ssize_t dfk_tcp_socket_read(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes);


/**
 * Read data from the socket into several buffers at once
 */
ssize_t dfk_tcp_socket_readv(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov);


/**
 * Write data to socket
 */
ssize_t dfk_tcp_socket_write(
    dfk_tcp_socket_t* sock,
    char* buf,
    size_t nbytes);


/**
 * Write data from several buffers to socket at once
 */
ssize_t dfk_tcp_socket_writev(
    dfk_tcp_socket_t* sock,
    dfk_iovec_t* iov,
    size_t niov);

/** @} */

