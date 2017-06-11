/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <dfk/tcp_socket.h>

static void connection(dfk_fiber_t* fiber, dfk_tcp_socket_t* sock, void* arg)
{
  (void) fiber;
  (void) arg;

  for (;;) {
    char buf[512] = {0};
    ssize_t nread = dfk_tcp_socket_read(sock, buf, sizeof(buf));
    if (nread <= 0) {
      break;
    }
    ssize_t nwritten = dfk_tcp_socket_write(sock, buf, nread);
    if (nwritten != nread) {
      break;
    }
  }

  (void) dfk_tcp_socket_close(sock);
}

static void dfkmain(dfk_fiber_t* fiber, void* arg)
{
  (void) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_tcp_socket_t sock;
  dfk_tcp_socket_init(&sock, dfk);
  dfk_tcp_socket_listen(&sock, "127.0.0.1", 20000, connection, NULL, 100);
}

int main(int argc, char** argv)
{
  (void) argc;
  (void) argv;
  dfk_t dfk;
  dfk_init(&dfk);
  dfk_work(&dfk, dfkmain, NULL, 0);
  dfk_free(&dfk);
  return 0;
}

