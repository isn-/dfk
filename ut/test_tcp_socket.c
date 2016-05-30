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
#include <dfk.h>
#include <dfk/internal.h>
#include "ut.h"
#include "naivetp/naivetp.h"


typedef struct
{
  dfk_t dfk;
  naivetp_server_t* ntp_server;
} echo_fixture_t;


static void echo_fixture_setup(echo_fixture_t* f)
{
  ASSERT_OK(dfk_init(&f->dfk));
  f->ntp_server = naivetp_server_start(&f->dfk, 10020);
  ASSERT(f->ntp_server);
}


static void echo_fixture_teardown(echo_fixture_t* f)
{
  naivetp_server_stop(f->ntp_server);
  ASSERT_OK(dfk_free(&f->dfk));
}


static void connect_disconnect(dfk_coro_t* coro, void* p)
{
  dfk_t* dfk = coro->dfk;
  int* connected = (int*) p;
  dfk_tcp_socket_t sock;
  ASSERT_OK(dfk_tcp_socket_init(&sock, dfk));
  ASSERT_OK(dfk_tcp_socket_connect(&sock, "127.0.0.1", 10020));
  *connected = 1;
  ASSERT_OK(dfk_tcp_socket_close(&sock));
  ASSERT_OK(dfk_tcp_socket_free(&sock));
}


TEST_F(echo_fixture, tcp_socket, connect_disconnect)
{
  int connected = 0;
  ASSERT(dfk_run(&fixture->dfk, connect_disconnect, &connected, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
  EXPECT(connected == 1);
}


static void single_write_read(dfk_coro_t* coro, void* p)
{
  dfk_t* dfk = coro->dfk;
  dfk_tcp_socket_t sock;
  DFK_UNUSED(p);
  ASSERT_OK(dfk_tcp_socket_init(&sock, dfk));
  ASSERT_OK(dfk_tcp_socket_connect(&sock, "127.0.0.1", 10020));
  {
    char buffer[64] = {0};
    size_t i;
    ssize_t nread;
    for (i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i) {
      buffer[i] = (char) (i + 24) % 256;
    }
    ASSERT(dfk_tcp_socket_write(&sock, buffer, sizeof(buffer)) == sizeof(buffer));
    memset(buffer, 0, sizeof(buffer));
    nread = dfk_tcp_socket_read(&sock, buffer, sizeof(buffer));
    ASSERT(nread > 0);
    for (i = 0; i < (size_t) nread; ++i) {
      ASSERT(buffer[i] == (char) (i + 24) % 256);
    }
  }
  ASSERT_OK(dfk_tcp_socket_close(&sock));
  ASSERT_OK(dfk_tcp_socket_free(&sock));
}


TEST_F(echo_fixture, tcp_socket, single_write_read)
{
  ASSERT(dfk_run(&fixture->dfk, single_write_read, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void multi_write_read(dfk_coro_t* coro, void* p)
{
  dfk_t* dfk = coro->dfk;
  dfk_tcp_socket_t sock;
  DFK_UNUSED(p);
  ASSERT_OK(dfk_tcp_socket_init(&sock, dfk));
  ASSERT_OK(dfk_tcp_socket_connect(&sock, "127.0.0.1", 10020));
  {
    char out[10240] = {0};
    char in[10240] = {0};
    ssize_t toread;
    size_t nread, i;
    for (i = 0; i < sizeof(out) / sizeof(out[0]); ++i) {
      out[i] = (char) (i + 24) % 256;
    }
    for (i = 0; i < 5; ++i) {
      ASSERT(dfk_tcp_socket_write(&sock, out + i * 1024, 1024) == 1024);
    }
    toread = 2048;
    while (toread > 0) {
      nread = dfk_tcp_socket_read(&sock, in + 2048 - toread, toread);
      ASSERT(nread > 0);
      toread -= nread;
    }
    ASSERT(toread == 0);
    toread = 2048;
    while (toread > 0) {
      nread = dfk_tcp_socket_read(&sock, in + 4096 - toread, toread);
      ASSERT(nread > 0);
      toread -= nread;
    }
    ASSERT(toread == 0);
    for (i = 5; i < 10; ++i) {
      ASSERT(dfk_tcp_socket_write(&sock, out + i * 1024, 1024) == 1024);
    }
    toread = sizeof(in) - 4096;
    while (toread > 0) {
      nread = dfk_tcp_socket_read(&sock, in + sizeof(in) - toread, toread);
      ASSERT(nread > 0);
      toread -= nread;
    }
    ASSERT(toread == 0);
    ASSERT(memcmp(in, out, sizeof(out)) == 0);
  }
  ASSERT_OK(dfk_tcp_socket_close(&sock));
  ASSERT_OK(dfk_tcp_socket_free(&sock));
}


TEST_F(echo_fixture, tcp_socket, multi_write_read)
{
  ASSERT(dfk_run(&fixture->dfk, multi_write_read, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


static void single_writev_readv(dfk_coro_t* coro, void* p)
{
  dfk_t* dfk = coro->dfk;
  dfk_tcp_socket_t sock;
  DFK_UNUSED(p);
  ASSERT_OK(dfk_tcp_socket_init(&sock, dfk));
  ASSERT_OK(dfk_tcp_socket_connect(&sock, "127.0.0.1", 10020));
  {
    char buffer[64] = {0};
    dfk_iovec_t chunks[3];
    size_t i;
    ssize_t nread;
    for (i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i) {
      buffer[i] = (char) (i + 24) % 256;
    }
    chunks[0].data = buffer;
    chunks[0].size = 32;
    chunks[1].data = buffer + 32;
    chunks[1].size = 16;
    chunks[2].data = buffer + 48;
    chunks[2].size = 16;
    ASSERT(dfk_tcp_socket_writev(&sock, chunks, 3) == sizeof(buffer));
    memset(buffer, 0, sizeof(buffer));
    nread = dfk_tcp_socket_readv(&sock, chunks, 3);
    ASSERT(nread > 0);
    for (i = 0; i < (size_t) nread; ++i) {
      ASSERT(buffer[i] == (char) (i + 24) % 256);
    }
  }
  ASSERT_OK(dfk_tcp_socket_close(&sock));
  ASSERT_OK(dfk_tcp_socket_free(&sock));
}


TEST_F(echo_fixture, tcp_socket, single_writev_readv)
{
  ASSERT(dfk_run(&fixture->dfk, single_writev_readv, NULL, 0));
  ASSERT_OK(dfk_work(&fixture->dfk));
}


typedef struct {
  dfk_t* dfk;
  const char* endpoint;
  uint16_t port;
  int connected;
} ut_connector_arg_t;


static void* ut_connector_start_stop(void* arg)
{
  ut_connector_arg_t* carg = (ut_connector_arg_t*) arg;
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
    DFK_DBG(carg->dfk, "try to connect, attempt %d", i);
    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) == 0) {
      DFK_DBG(carg->dfk, "connected");
      carg->connected += 1;
      break;
    }
    /* wait for 1, 4, 9, 16, 25, ... , 1024 milliseconds */
    memset(&req, 0, sizeof(req));
    memset(&rem, 0, sizeof(rem));
    req.tv_nsec = i * i * 1000000;
    DFK_DBG(carg->dfk, "connect attempt failed, retry in %d msec", i * i);
    nanosleep(&req, &rem);
  }
  close(sockfd);
  pthread_exit(NULL);
  return NULL;
}


static void on_new_connection_close(dfk_coro_t* coro, dfk_tcp_socket_t* sock, void* cbarg)
{
  ASSERT(coro != NULL);
  ASSERT(sock != NULL);
  ASSERT(cbarg != NULL);
  ASSERT_OK(dfk_tcp_socket_close(sock));
  ASSERT_OK(dfk_tcp_socket_close((dfk_tcp_socket_t*) cbarg));
}


static void ut_listen_start_stop(dfk_coro_t* coro, void* p)
{
  dfk_tcp_socket_t sock;
  DFK_UNUSED(p);
  ASSERT_OK(dfk_tcp_socket_init(&sock, coro->dfk));
  ASSERT_OK(dfk_tcp_socket_listen(&sock, "127.0.0.1", 10000, on_new_connection_close, &sock, 0));
  ASSERT_OK(dfk_tcp_socket_free(&sock));
}


TEST(tcp_socket, listen_start_stop)
{
  pthread_t cthread;
  ut_connector_arg_t carg;
  dfk_t dfk;

  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, ut_listen_start_stop, NULL, 0));

  carg.dfk = &dfk;
  carg.port = 10000;
  carg.endpoint = "127.0.0.1";
  carg.connected = 0;
  ASSERT(!pthread_create(&cthread, NULL, &ut_connector_start_stop, &carg));

  ASSERT_OK(dfk_work(&dfk));
  ASSERT_OK(dfk_free(&dfk));

  ASSERT(pthread_join(cthread, NULL) == 0);
  ASSERT(carg.connected == 1);
}


static void* ut_connector_read_write(void* arg)
{
  ut_connector_arg_t* carg = (ut_connector_arg_t*) arg;
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
    DFK_DBG(carg->dfk, "try to connect, attempt %d", i);
    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) == 0) {
      DFK_DBG(carg->dfk, "connected");
      carg->connected += 1;
      break;
    }
    /* wait for 1, 4, 9, 16, 25, ... , 1024 milliseconds */
    memset(&req, 0, sizeof(req));
    memset(&rem, 0, sizeof(rem));
    req.tv_nsec = i * i * 1000000;
    DFK_DBG(carg->dfk, "connect attempt failed, retry in %d msec", i * i);
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


static void on_new_connection_echo(dfk_coro_t* coro, dfk_tcp_socket_t* sock, void* cbarg)
{
  ASSERT(coro != NULL);
  ASSERT(sock != NULL);
  ASSERT(cbarg != NULL);

  DFK_UNUSED(cbarg);

  for (;;) {
    char buf[512] = {0};
    ssize_t nbytes = dfk_tcp_socket_read(sock, buf, sizeof(buf));
    if (nbytes < 0) {
      break;
    }
    nbytes = dfk_tcp_socket_write(sock, buf, nbytes);
    if (nbytes < 0) {
      break;
    }
  }

  ASSERT_OK(dfk_tcp_socket_close(sock));
  ASSERT_OK(dfk_tcp_socket_close((dfk_tcp_socket_t*) cbarg));
}


static void ut_listen_read_write(dfk_coro_t* coro, void* p)
{
  dfk_tcp_socket_t sock;
  DFK_UNUSED(p);
  ASSERT_OK(dfk_tcp_socket_init(&sock, coro->dfk));
  ASSERT_OK(dfk_tcp_socket_listen(&sock, "127.0.0.1", 10000, on_new_connection_echo, &sock, 0));
  ASSERT_OK(dfk_tcp_socket_free(&sock));
}



TEST(tcp_socket, listen_read_write)
{
  pthread_t cthread;
  ut_connector_arg_t carg;
  dfk_t dfk;

  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, ut_listen_read_write, NULL, 0));

  carg.dfk = &dfk;
  carg.port = 10000;
  carg.endpoint = "127.0.0.1";
  carg.connected = 0;
  ASSERT(!pthread_create(&cthread, NULL, &ut_connector_read_write, &carg));

  ASSERT_OK(dfk_work(&dfk));
  ASSERT_OK(dfk_free(&dfk));

  ASSERT(pthread_join(cthread, NULL) == 0);
  ASSERT(carg.connected == 1);
}

