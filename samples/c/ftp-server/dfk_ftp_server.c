/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/tcp_server.h>
#include <dfk/signal.h>

/*
 * Basic implementation of the FTP protocol, defined in ftp_protocol.c
 *
 * The function uses basic dfk API to read/write from/to TCP socket,
 * namely functions:
 * * dfk_tcp_socket_read()
 * * dfk_tcp_socket_write()
 *
 * @warning Do not use in production!
 */
extern void ftp_connection(dfk_tcp_server_t* server, dfk_fiber_t* fiber,
    dfk_tcp_socket_t* sock, dfk_userdata_t arg);

static void wait_for_terminate(dfk_fiber_t* fiber, void* arg)
{
  (void) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_sigwait2(dfk, SIGINT, SIGTERM);
  dfk_stop(dfk);
}

static void dfkmain(dfk_fiber_t* fiber, void* arg)
{
  (void) arg;
  dfk_t* dfk = fiber->dfk;
  dfk_tcp_server_t server;
  dfk_tcp_server_init(&server, dfk);
  dfk_run(dfk, wait_for_terminate, NULL, 0);
  dfk_tcp_serve(&server, "127.0.0.1", 10021,
      ftp_connection, (dfk_userdata_t) {.data = NULL}, 128);
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

