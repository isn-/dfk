/**
 * @copyright
 * Copyright (c) 2015, 2016, Stanislav Ivochkin. All Rights Reserved.
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <assert.h>
#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/http.h>
#include "ut.h"


#if DFK_MOCKS


typedef struct fixture_t {
  dfk_t dfk;
  dfk_arena_t conn_arena;
  dfk_arena_t req_arena;
  dfk_tcp_socket_t sock;
  dfk_http_resp_t resp;
  dfk_sponge_t respbuf;
} fixture_t;


static void fixture_setup(fixture_t* f)
{
  EXPECT_OK(dfk_init(&f->dfk));
  EXPECT_OK(dfk_arena_init(&f->conn_arena, &f->dfk));
  EXPECT_OK(dfk_arena_init(&f->req_arena, &f->dfk));
  dfk__http_resp_init(&f->resp, &f->dfk, &f->req_arena, &f->conn_arena, &f->sock);
  dfk_sponge_init(&f->respbuf, &f->dfk);
  f->resp._sock_mocked = 1;
  f->resp._sock_mock = &f->respbuf;
  f->resp.major_version = 1;
  f->resp.minor_version = 0;
  f->resp.code = 200;
}


static void fixture_teardown(fixture_t* f)
{
  dfk_sponge_free(&f->respbuf);
  dfk__http_resp_free(&f->resp);
  EXPECT_OK(dfk_arena_free(&f->conn_arena));
  EXPECT_OK(dfk_arena_free(&f->req_arena));
  EXPECT_OK(dfk_free(&f->dfk));
}


static void expect_resp(dfk_http_resp_t* resp, const char* expected)
{
  dfk__http_resp_flush(resp);
  size_t actuallen = resp->_sock_mock->size;
  char* actual = DFK_MALLOC(resp->dfk, actuallen);
  EXPECT(actual);
  EXPECT(dfk_sponge_read(resp->_sock_mock, actual, actuallen) == (ssize_t) actuallen);
  size_t expectedlen = strlen(expected);
  EXPECT(actuallen == expectedlen);
  EXPECT(!strncmp(actual, expected, expectedlen));
  DFK_FREE(resp->dfk, actual);
}


TEST_F(fixture, http_resp, version)
{
  fixture->resp.major_version = 2;
  fixture->resp.minor_version = 1;
  expect_resp(&fixture->resp,
      "HTTP/2.1 200 OK\r\n");
}


TEST_F(fixture, http_resp, code)
{
  fixture->resp.code = 404;
  expect_resp(&fixture->resp,
      "HTTP/1.0 404 Not Found\r\n");
}


TEST_F(fixture, http_resp, set_one)
{
  EXPECT_OK(dfk_http_set(&fixture->resp, "Foo", 3, "bar", 3));
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Foo: bar\r\n");
}


TEST_F(fixture, http_resp, set_many)
{
  EXPECT_OK(dfk_http_set(&fixture->resp, "Foo", 3, "bar", 3));
  EXPECT_OK(dfk_http_set(&fixture->resp, "Quazu", 5, "v", 1));
  EXPECT_OK(dfk_http_set(&fixture->resp, "Some-Header", 11, "With some spaces", 16));
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Some-Header: With some spaces\r\n"
      "Quazu: v\r\n"
      "Foo: bar\r\n");
}


TEST_F(fixture, http_resp, set_copy)
{
  char h[] = "Foo";
  char v[] = "Value";
  EXPECT_OK(dfk_http_set_copy(&fixture->resp, h, sizeof(h) - 1, v, sizeof(v) - 1));
  h[0] = 'a';
  v[0] = 'b';
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Foo: Value\r\n");
}


TEST_F(fixture, http_resp, set_copy_name)
{
  char h[] = "Foo";
  char v[] = "Value";
  EXPECT_OK(dfk_http_set_copy_name(&fixture->resp, h, sizeof(h) - 1, v, sizeof(v) - 1));
  h[0] = 'a';
  v[0] = 'b';
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Foo: balue\r\n");
}


TEST_F(fixture, http_resp, set_copy_value)
{
  char h[] = "Foo";
  char v[] = "Value";
  EXPECT_OK(dfk_http_set_copy_value(&fixture->resp, h, sizeof(h) - 1, v, sizeof(v) - 1));
  h[0] = 'a';
  v[0] = 'b';
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "aoo: Value\r\n");
}


TEST_F(fixture, http_resp, set_errors)
{
  EXPECT(dfk_http_set(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy_name(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy_name(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy_name(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy_value(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy_value(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_set_copy_value(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
}

#endif /* DFK_MOCKS */

