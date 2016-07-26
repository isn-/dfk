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
#include <stdio.h>
#include <dfk.h>


typedef struct args_t {
  int argc;
  char** argv;
} args_t;


#define CALL_DFK_API(c) if ((c) != dfk_err_ok) { return -1; }


static int echo(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  dfk_http_header_it it;
  (void) http;
  CALL_DFK_API(dfk_http_request_headers_begin(req, &it));
  while (dfk_http_headers_valid(&it) == dfk_err_ok) {
    dfk_buf_t name = it.header->name;
    dfk_buf_t value = it.header->value;
    CALL_DFK_API(dfk_http_set(resp, name.data, name.size, value.data, value.size));
    CALL_DFK_API(dfk_http_headers_next(&it));
  }
  resp->code = DFK_HTTP_OK;
  return dfk_err_ok;;
}


static void dfk_main(dfk_coro_t* coro, void* p)
{
  dfk_http_t srv;
  args_t* args = (args_t*) p;
  if (dfk_http_init(&srv, coro->dfk) != dfk_err_ok) {
    return;
  }
  if (dfk_http_serve(&srv, args->argv[1], atoi(args->argv[2]), echo) != dfk_err_ok) {
    return;
  }
  dfk_http_free(&srv);
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
  CALL_DFK_API(dfk_work(&dfk));
  return dfk_free(&dfk);
}

