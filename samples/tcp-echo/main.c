/**
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
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

#include <stdio.h>
#include <stdlib.h>
#include <dfk.h>
#include <dfk/internal.h>


typedef struct args_t {
  int argc;
  char** argv;
} args_t;


static void connection_handler(dfk_coro_t* coro, dfk_tcp_socket_t* sock, void* p)
{
  char buf[512] = {0};
  ssize_t nread, nwritten;

  DFK_UNUSED(coro);
  DFK_UNUSED(p);

  for (;;) {
    nread = dfk_tcp_socket_read(sock, buf, sizeof(buf));
    if (nread < 0) {
      break;
    }
    nwritten = dfk_tcp_socket_write(sock, buf, nread);
    if (nwritten != nread) {
      break;
    }
  }

  (void) dfk_tcp_socket_close(sock);
}


static void dfk_main(dfk_coro_t* coro, void* p)
{
  dfk_tcp_socket_t sock;
  args_t* args = (args_t*) p;
  DFK_CALL_RVOID(coro->dfk, dfk_tcp_socket_init(&sock, coro->dfk));
  DFK_CALL_RVOID(coro->dfk,
      dfk_tcp_socket_listen(&sock, args->argv[1], atoi(args->argv[2]), connection_handler, NULL, 0));
}


int main(int argc, char** argv)
{
  dfk_t dfk;
  args_t args;
  if (argc != 3) {
    fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
    return 1;
  }
  args.argc = argc;
  args.argv = argv;
  dfk_init(&dfk);
  (void) dfk_run(&dfk, dfk_main, &args, 0);
  DFK_CALL(&dfk, dfk_work(&dfk));
  return dfk_free(&dfk);
}

