/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdlib.h>
#include <dfk.h>

static const char* ip;
static unsigned int port;

static void dfk_main(dfk_coro_t* coro, void* p)
{
  (void) p;
  dfk_http_t srv;
  dfk_fileserver_t fs;
  dfk_fileserver_init(&fs, coro->dfk, ".", 1);
  dfk_userdata_t ud = {&fs};
  if (dfk_http_init(&srv, coro->dfk) != dfk_err_ok) {
    return;
  }
  if (dfk_http_serve(&srv, ip, port, dfk_fileserver_handler, ud) != dfk_err_ok) {
    return;
  }
  dfk_fileserver_free(&fs);
  dfk_http_free(&srv);
}

int main(int argc, char** argv)
{
  if (argc != 3) {
    fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
    return -1;
  }
  ip = argv[1];
  port = atoi(argv[2]);
  dfk_t dfk;
  dfk_init(&dfk);
  (void) dfk_run(&dfk, dfk_main, NULL, 0);
  if (dfk_work(&dfk) != dfk_err_ok) {
    return -1;
  }
  return dfk_free(&dfk);
}

