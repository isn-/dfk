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
#include <time.h>
#include <pthread.h>
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

typedef struct {
  const char* endpoint;
  uint16_t port;
} connector_arg;

static void* connector_start_stop(void* arg)
{
  connector_arg* carg = (connector_arg*) arg;
  int sockfd, i;
  struct sockaddr_in addr;
  struct timespec req, rem;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_RET(sockfd != -1, NULL);
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(carg->port);
  inet_aton(carg->endpoint, (struct in_addr*) &addr.sin_addr.s_addr);
  for (i = 0; i < 32; ++i) {
    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) == 0) {
      break;
    }
    /* wait for 1, 4, 9, 16, 25, ... , 1024 milliseconds */
    req.tv_nsec = i * i * 1000000;
    nanosleep(&req, &rem);
  }
  close(sockfd);
  return NULL;
}

static void on_new_connection_close(dfk_tcp_socket_t* lsock, dfk_tcp_socket_t* sock, int err)
{
  ASSERT(lsock != NULL);
  ASSERT(sock != NULL);
  ASSERT(err == dfk_err_ok);
  *((int*) (lsock->userdata)) += 1;
  ASSERT_OK(dfk_tcp_socket_close(lsock));
  ASSERT_OK(dfk_tcp_socket_close(sock));
}

TEST(tcp_socket, listen_start_stop)
{
  int nconnected = 0;
  pthread_t cthread;
  connector_arg carg;
  dfk_context_t* ctx = dfk_default_context();
  dfk_tcp_socket_t sock;
  dfk_event_loop_t loop;
  ASSERT_OK(dfk_event_loop_init(&loop, ctx));
  ASSERT_OK(dfk_tcp_socket_init(&sock, &loop));
  ASSERT_OK(dfk_tcp_socket_start_listen(
    &sock, "127.0.0.1", 10000, on_new_connection_close, 0));
  sock.userdata = &nconnected;
  carg.port = 10000;
  carg.endpoint = "127.0.0.1";
  ASSERT(pthread_create(&cthread, NULL, &connector_start_stop, &carg) == 0);
  ASSERT_OK(dfk_event_loop_run(&loop));
  ASSERT_OK(dfk_event_loop_join(&loop));
  ASSERT(nconnected == 1);
  ASSERT(pthread_join(cthread, NULL) == 0);
}

static void* connector_read_write(void* arg)
{
  connector_arg* carg = (connector_arg*) arg;
  int sockfd;
  unsigned int i;
  ssize_t nread = 0, nwritten = 0;
  size_t totalread = 0;
  struct sockaddr_in addr;
  struct timespec req, rem;
  unsigned char buf[1234] = {0};

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_RET(sockfd != -1, NULL);
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(carg->port);
  inet_aton(carg->endpoint, (struct in_addr*) &addr.sin_addr.s_addr);
  for (i = 0; i < 32; ++i) {
    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) == 0) {
      break;
    }
    /* wait for 1, 4, 9, 16, 25, ... , 1024 milliseconds */
    req.tv_nsec = i * i * 1000000;
    nanosleep(&req, &rem);
  }
  for (i = 0; i < sizeof(buf); ++i) {
    buf[i] = i % 256;
  }
  nwritten = write(sockfd, buf, sizeof(buf));
  ASSERT_RET(nwritten == sizeof(buf), NULL);
  memset(buf, 0, sizeof(buf));
  do {
    nread = read(sockfd, buf + totalread, sizeof(buf) - totalread);
    ASSERT_RET(nread > 0, NULL);
    totalread += nread;
  } while (totalread < sizeof(buf));
  for (i = 0; i < sizeof(buf); ++i) {
    ASSERT_RET(buf[i] == i % 256, NULL);
  }
  close(sockfd);
  return NULL;
}

static void on_new_connection_echo(dfk_tcp_socket_t* lsock, dfk_tcp_socket_t* sock, int err)
{
  char buf[512] = {0};
  size_t nread;

  ASSERT(lsock != NULL);
  ASSERT(sock != NULL);
  ASSERT(err == dfk_err_ok);
  (void) lsock;

  for (;;) {
    err = dfk_tcp_socket_read(sock, buf, sizeof(buf), &nread);
    if (err != dfk_err_ok) {
      break;
    }
    err = dfk_tcp_socket_write(sock, buf, nread);
    if (err != dfk_err_ok) {
      break;
    }
  }

  (void) dfk_tcp_socket_close(sock);
  (void) dfk_tcp_socket_close(lsock);
}

TEST(tcp_socket, listen_read_write)
{
  int nconnected = 0;
  pthread_t cthread;
  connector_arg carg;
  dfk_context_t* ctx = dfk_default_context();
  dfk_tcp_socket_t sock;
  dfk_event_loop_t loop;
  ASSERT_OK(dfk_event_loop_init(&loop, ctx));
  ASSERT_OK(dfk_tcp_socket_init(&sock, &loop));
  ASSERT_OK(dfk_tcp_socket_start_listen(
    &sock, "127.0.0.1", 10000, on_new_connection_echo, 0));
  sock.userdata = &nconnected;
  carg.port = 10000;
  carg.endpoint = "127.0.0.1";
  ASSERT(pthread_create(&cthread, NULL, &connector_read_write, &carg) == 0);
  ASSERT_OK(dfk_event_loop_run(&loop));
  ASSERT_OK(dfk_event_loop_join(&loop));
  ASSERT(nconnected == 1);
  ASSERT(pthread_join(cthread, NULL) == 0);
}

