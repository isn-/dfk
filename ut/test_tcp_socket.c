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

#include <unistd.h>
#include "ut.h"
#include <dfk/context.h>
#include <dfk/tcp_socket.h>
#include <dfk/event_loop.h>
#include "naivetp/naivetp.h"


typedef struct
{
  naivetp_server_t* ntp_server;
  dfk_context_t ctx;
  dfk_event_loop_t loop;
  dfk_tcp_socket_t sock;
  dfk_coro_t coro;
  void (*connect_callback)(dfk_tcp_socket_t*);
} echo_fixture;


static void echo_fixture_on_connect(dfk_tcp_socket_t* sock)
{
  echo_fixture* fixture = (echo_fixture*) ((char*) sock - offsetof(echo_fixture, sock));
  fixture->connect_callback(sock);
}


static void echo_fixture_setup(echo_fixture* f)
{
  ASSERT_OK(dfk_context_init(&f->ctx));
  f->ntp_server = naivetp_server_start(&f->ctx, 10020);
  ASSERT_OK(dfk_event_loop_init(&f->loop, &f->ctx));
  ASSERT_OK(dfk_tcp_socket_init(&f->sock, &f->loop));
  ASSERT_OK(dfk_coro_init(&f->coro, &f->ctx, 0));
  ASSERT_OK(dfk_tcp_socket_start_connect(
      &f->sock, "127.0.0.1", 10020, echo_fixture_on_connect, &f->coro));
}


static void echo_fixture_teardown(echo_fixture* f)
{
  ASSERT_OK(dfk_tcp_socket_free(&f->sock));
  ASSERT_OK(dfk_event_loop_free(&f->loop));
  ASSERT_OK(dfk_context_free(&f->ctx));
  naivetp_server_stop(f->ntp_server);
}


static void connect_disconnect(dfk_tcp_socket_t* sock)
{
  *((int*) sock->userdata) = 1;
  dfk_tcp_socket_close(sock);
}


TEST_F(echo_fixture, tcp_socket, connect_disconnect)
{
  int connected = 0;

  fixture->connect_callback = connect_disconnect;
  fixture->sock.userdata = &connected;
  ASSERT_OK(dfk_event_loop_run(&fixture->loop));
  ASSERT_OK(dfk_event_loop_join(&fixture->loop));
  EXPECT(connected == 1);
}


static void single_write_read(dfk_tcp_socket_t* sock)
{
  char buffer[64] = {0};
  size_t nread, i;
  for (i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i) {
    buffer[i] = (char) (i + 24) % 256;
  }
  ASSERT_OK(dfk_tcp_socket_write(sock, buffer, sizeof(buffer)));
  memset(buffer, 0, sizeof(buffer));
  ASSERT_OK(dfk_tcp_socket_read(sock, buffer, sizeof(buffer), &nread));
  ASSERT(nread == sizeof(buffer));
  for (i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i) {
    ASSERT(buffer[i] == (char) (i + 24) % 256);
  }
  ASSERT_OK(dfk_tcp_socket_close(sock));
}


TEST_F(echo_fixture, tcp_socket, single_write_read)
{
  fixture->connect_callback = &single_write_read;
  ASSERT_OK(dfk_event_loop_run(&fixture->loop));
  ASSERT_OK(dfk_event_loop_join(&fixture->loop));
}


static void multi_write_read(dfk_tcp_socket_t* sock)
{
  char out[10240] = {0};
  char in[10240] = {0};
  ssize_t toread;
  size_t nread, i;
  for (i = 0; i < sizeof(out) / sizeof(out[0]); ++i) {
    out[i] = (char) (i + 24) % 256;
  }
  for (i = 0; i < 5; ++i) {
    ASSERT_OK(dfk_tcp_socket_write(sock, out + i * 1024, 1024));
  }
  toread = 2048;
  while (toread > 0) {
    ASSERT_OK(dfk_tcp_socket_read(sock, in + 2048 - toread, toread, &nread));
    toread -= nread;
  }
  ASSERT(toread == 0);
  toread = 2048;
  while (toread > 0) {
    ASSERT_OK(dfk_tcp_socket_read(sock, in + 4096 - toread, toread, &nread));
    toread -= nread;
  }
  ASSERT(toread == 0);
  for (i = 5; i < 10; ++i) {
    ASSERT_OK(dfk_tcp_socket_write(sock, out + i * 1024, 1024));
  }
  toread = sizeof(in) - 4096;
  while (toread > 0) {
    ASSERT_OK(dfk_tcp_socket_read(sock, in + sizeof(in) - toread, toread, &nread));
    toread -= nread;
  }
  ASSERT(toread == 0);
  ASSERT(memcmp(in, out, sizeof(out)) == 0);
  ASSERT_OK(dfk_tcp_socket_close(sock));
}


TEST_F(echo_fixture, tcp_socket, multi_write_read)
{
  fixture->connect_callback = multi_write_read;
  ASSERT_OK(dfk_event_loop_run(&fixture->loop));
  ASSERT_OK(dfk_event_loop_join(&fixture->loop));
}


static void read_all(dfk_tcp_socket_t* sock)
{
  char out[10240] = {0};
  char in[10240] = {0};
  size_t nread, i;
  for (i = 0; i < sizeof(out) / sizeof(out[0]); ++i) {
    out[i] = (char) (i + 24) % 256;
  }
  for (i = 0; i < 6; ++i) {
    ASSERT_OK(dfk_tcp_socket_write(sock, out + i * 1024, 1024));
  }
  ASSERT_OK(dfk_tcp_socket_readall(sock, in, 6 * 1024, &nread));
  ASSERT(nread == 6 * 1024);
  for (i = 6; i < 10; ++i) {
    ASSERT_OK(dfk_tcp_socket_write(sock, out + i * 1024, 1024));
  }
  ASSERT_OK(dfk_tcp_socket_readall(sock, in + 6 * 1024, 4 * 1024, &nread));
  ASSERT(nread == 4 * 1024);
  ASSERT(memcmp(in, out, sizeof(out)) == 0);
  ASSERT_OK(dfk_tcp_socket_close(sock));
}


TEST_F(echo_fixture, tcp_socket, read_all)
{
  fixture->connect_callback = read_all;
  ASSERT_OK(dfk_event_loop_run(&fixture->loop));
  ASSERT_OK(dfk_event_loop_join(&fixture->loop));
}

