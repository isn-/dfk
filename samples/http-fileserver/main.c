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

