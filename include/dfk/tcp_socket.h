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

#pragma once
#include <stdint.h>
#include <sys/types.h>
#include <uv.h>
#include <dfk/core.h>


/**
 * @file dfk/tcp_socket.h
 * TCP socket object and related functions
 *
 * @defgroup tcp_socket tcp_socket
 * @{
 */

/**
 * TCP socket object
 */
typedef struct dfk_tcp_socket_t {
  struct {
    uv_tcp_t socket;
    /*
     * Compilers complain on casting function pointer to void*, so we
     * use a separate union member when function pointer is required.
     */
    union {
      void* obj;
      void (*func)(void*);
    } arg;
    int32_t flags;
  } _;
  void* userdata;
  dfk_t* dfk;
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
    void (*callback)(dfk_t*, dfk_tcp_socket_t*, int),
    size_t backlog);


/**
 * Close socket. Any operation currently running will be interrupted
 */
int dfk_tcp_socket_close(dfk_tcp_socket_t* sock);


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

