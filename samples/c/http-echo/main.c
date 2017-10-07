/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdlib.h>
#include <stdio.h>
#include <dfk/error.h>
#include <dfk/http/server.h>

typedef struct args_t {
  int argc;
  char** argv;
} args_t;

static int echo(dfk_http_t* http, dfk_http_request_t* req,
    dfk_http_response_t* resp, dfk_userdata_t ud)
{
  (void) ud;
  (void) http;

  /* Copy HTTP headers */
  dfk_strmap_it it, end;
  dfk_strmap_begin(&req->headers, &it);
  dfk_strmap_end(&req->headers, &end);
  while (!dfk_strmap_it_equal(&it, &end)) {
    dfk_cbuf_t name = it.item->key;
    dfk_buf_t value = it.item->value;
    dfk_http_response_set(resp, name.data, name.size, value.data, value.size);
    dfk_strmap_it_next(&it);
  }

  /* Copy request body */
  if (req->content_length > 0) {
    char buf[4096] = {0};
    ssize_t nread = 0;
    while (nread >= 0) {
      nread = dfk_http_request_read(req, buf, sizeof(buf));
      if (nread < 0) {
        return -1;
      }
      ssize_t nwritten = dfk_http_response_write(resp, buf, nread);
      if (nwritten < 0) {
        return -1;
      }
    }
  }

  resp->status = DFK_HTTP_OK;
  return dfk_err_ok;;
}

static void dfkmain(dfk_fiber_t* fiber, void* p)
{
  (void) p;
  dfk_http_t srv;
  dfk_http_init(&srv, fiber->dfk);
  dfk_http_serve(&srv, "127.0.0.1", 10080, 128, echo, (dfk_userdata_t) {.data = NULL});
  dfk_http_free(&srv);
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

