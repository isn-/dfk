/**
 * @file dfk/tcp_socket.h
 * @brief TCP socket object and related functions
 *
 * @author Stanislav Ivochkin
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
#include <uv.h>
#include <dfk/core.h>
#include <dfk/buf.h>
#include <dfk/event_loop.h>
#include <dfk/coro.h>

/**
 * @brief TCP socket object
 * @test All the API
 */
typedef struct {
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
} dfk_tcp_socket_t;

/**
 * @brief Initialize tcp_socket object
 * @param obj an object for in-place initialization
 * @param loop pointer to valid event loop object
 * @return
 *   @li dfk_err_ok
 *   @li dfk_err_badarg
 *   @li dfk_err_sys
 */
int dfk_tcp_socket_init(dfk_tcp_socket_t* obj, dfk_event_loop_t* loop);

/**
 * @brief Cleaup resources allocated for tcp_socket object
 */
int dfk_tcp_socket_free(dfk_tcp_socket_t* obj);

/**
 * @brief Enqueue TCP connect request
 * @note This function should be called during the event loop initialization
 * step, before dfk_event_loop_run function. After the event loop is started,
 * a call to this function will return dfk_err_coro.
 * @return
 *  @li dfk_err_ok
 *  @li dfk_err_badarg
 *  @li dfk_err_inprog
 *  @li dfk_err_sys
 *  @li dfk_err_out_of_memory
 */
int dfk_tcp_socket_start_connect(
    dfk_tcp_socket_t* obj,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_tcp_socket_t*, int));

/**
 * @brief Connect to TCP endpoint
 * @note This function should be called within a running coroutine.
 */
int dfk_tcp_socket_connect(
    dfk_tcp_socket_t* obj,
    const char* endpoint,
    uint16_t port);

/**
 * @brief Enqueue listen request
 * @see dfk_tcp_socket_listen
 * @note This function should be called during the event loop initialization
 * step, before dfk_event_loop_run function. After the event loop is started,
 * a call to this function will return dfk_err_coro.
 */
int dfk_tcp_socket_start_listen(
    dfk_tcp_socket_t* obj,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_tcp_socket_t*, int),
    size_t backlog);

/**
 * @brief Listen for connections on endpoint:port.
 * @detail callback is executed for each incoming connection
 * @note To stop listening, call dfk_tcp_socket_close
 */
int dfk_tcp_socket_listen(
    dfk_tcp_socket_t* obj,
    const char* endpoint,
    uint16_t port,
    void (*callback)(dfk_tcp_socket_t*),
    size_t backlog);

/**
 * @brief Close socket. Any operation currently running will be interrupted
 */
int dfk_tcp_socket_close(dfk_tcp_socket_t* obj);

/**
 * @brief Read data from the socket
 */
int dfk_tcp_socket_read(
    dfk_tcp_socket_t* obj,
    char* buf,
    size_t nbytes,
    size_t* nread);

/**
 * @todo Implement it
 */
int dfk_tcp_socket_readv(
    dfk_tcp_socket_t* obj,
    dfk_iovec_t* iov,
    size_t niov,
    size_t *nread);

/**
 * @brief Write data to socket
 */
int dfk_tcp_socket_write(
    dfk_tcp_socket_t* obj,
    char* buf,
    size_t nbytes);

/**
 * @todo Implement it
 */
int dfk_tcp_socket_writev(
    dfk_tcp_socket_t* obj,
    dfk_iovec_t* iov,
    size_t niov);

