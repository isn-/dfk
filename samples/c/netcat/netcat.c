/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */


#include <stdio.h>
#include <dfk/context.h>
#include <dfk/fiber.h>
#include <dfk/tcp_socket.h>

static void dfkmain(dfk_fiber_t* fiber, void* arg)
{
  (void) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_tcp_socket_t sock;
  dfk_tcp_socket_init(&sock, dfk);
  dfk_tcp_socket_connect(&sock, "127.0.0.1", 20000);
  ssize_t nread;
  do {
    char buf[1024];
    nread = dfk_tcp_socket_read(&sock, buf, sizeof(buf));
    if (nread > 0) {
      printf("%.*s", (int) nread, buf);
    }
  } while (nread > 0);
  dfk_tcp_socket_close(&sock);
}

int main(int argc, char** argv)
{
  (void) argc;
  (void) argv;
  dfk_t dfk;
  dfk_init(&dfk);
  dfk_work(&dfk, dfkmain, NULL, 0);
  return 0;
}

