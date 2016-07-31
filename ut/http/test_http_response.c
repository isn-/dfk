/**
 * @copyright
 * Copyright (c) 2015-2016 Stanislav Ivochkin
 *
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
#include <dfk/internal.h>
#include <ut.h>


/*
 * The following tests are using dfk_http_response._sock mock,
 * so if DFK_MOCKS are disabled, these tests does not make sense
 */
#if DFK_MOCKS


typedef struct fixture_t {
  dfk_t dfk;
  dfk_arena_t conn_arena;
  dfk_arena_t req_arena;
  dfk_tcp_socket_t sock;
  dfk_http_response_t resp;
  dfk_sponge_t respbuf;
} fixture_t;


static void fixture_setup(fixture_t* f)
{
  EXPECT_OK(dfk_init(&f->dfk));
  EXPECT_OK(dfk_arena_init(&f->conn_arena, &f->dfk));
  EXPECT_OK(dfk_arena_init(&f->req_arena, &f->dfk));
  EXPECT_OK(dfk_http_response_init(&f->resp, &f->dfk, &f->req_arena,
                                   &f->conn_arena, &f->sock));
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
  EXPECT_OK(dfk_http_response_free(&f->resp));
  EXPECT_OK(dfk_arena_free(&f->conn_arena));
  EXPECT_OK(dfk_arena_free(&f->req_arena));
  EXPECT_OK(dfk_free(&f->dfk));
}


static void expect_resp(dfk_http_response_t* resp, const char* expected)
{
  dfk_http_response_flush_headers(resp);
  size_t actuallen = resp->_sock_mock->size;
  char* actual = DFK_MALLOC(resp->dfk, actuallen);
  EXPECT(actual);
  EXPECT(dfk_sponge_read(resp->_sock_mock, actual, actuallen) == (ssize_t) actuallen);
  size_t expectedlen = strlen(expected);
  EXPECT(actuallen == expectedlen);
  EXPECT(!strncmp(actual, expected, expectedlen));
  DFK_FREE(resp->dfk, actual);
}


TEST_F(fixture, http_response, version)
{
  fixture->resp.major_version = 2;
  fixture->resp.minor_version = 1;
  expect_resp(&fixture->resp,
      "HTTP/2.1 200 OK\r\n\r\n");
}


TEST_F(fixture, http_response, code)
{
  fixture->resp.code = 404;
  expect_resp(&fixture->resp,
      "HTTP/1.0 404 Not Found\r\n\r\n");
}


TEST_F(fixture, http_response, set_one)
{
  EXPECT_OK(dfk_http_response_set(&fixture->resp, "Foo", 3, "bar", 3));
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Foo: bar\r\n\r\n");
}


TEST_F(fixture, http_response, set_many)
{
  EXPECT_OK(dfk_http_response_set(&fixture->resp, "Foo", 3, "bar", 3));
  EXPECT_OK(dfk_http_response_set(&fixture->resp, "Quazu", 5, "v", 1));
  EXPECT_OK(dfk_http_response_set(&fixture->resp, "Some-Header", 11, "With some spaces", 16));
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Some-Header: With some spaces\r\n"
      "Quazu: v\r\n"
      "Foo: bar\r\n\r\n");
}


TEST_F(fixture, http_response, set_copy)
{
  char h[] = "Foo";
  char v[] = "Value";
  EXPECT_OK(dfk_http_response_set_copy(&fixture->resp, h, sizeof(h) - 1, v, sizeof(v) - 1));
  h[0] = 'a';
  v[0] = 'b';
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Foo: Value\r\n\r\n");
}


TEST_F(fixture, http_response, set_copy_name)
{
  char h[] = "Foo";
  char v[] = "Value";
  EXPECT_OK(dfk_http_response_set_copy_name(&fixture->resp, h, sizeof(h) - 1, v, sizeof(v) - 1));
  h[0] = 'a';
  v[0] = 'b';
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Foo: balue\r\n\r\n");
}


TEST_F(fixture, http_response, set_copy_value)
{
  char h[] = "Foo";
  char v[] = "Value";
  EXPECT_OK(dfk_http_response_set_copy_value(&fixture->resp, h, sizeof(h) - 1, v, sizeof(v) - 1));
  h[0] = 'a';
  v[0] = 'b';
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "aoo: Value\r\n\r\n");
}


TEST_F(fixture, http_response, set_errors)
{
  EXPECT(dfk_http_response_set(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_ALWAYS, 0);
  EXPECT(dfk_http_response_set(&fixture->resp, "n", 1, "v", 1) == dfk_err_nomem);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_NEVER, 0);

  EXPECT(dfk_http_response_set_copy(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set_copy(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set_copy(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_ALWAYS, 0);
  EXPECT(dfk_http_response_set_copy(&fixture->resp, "n", 1, "v", 1) == dfk_err_nomem);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_NEVER, 0);

  EXPECT(dfk_http_response_set_copy_name(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set_copy_name(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set_copy_name(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_ALWAYS, 0);
  EXPECT(dfk_http_response_set_copy_name(&fixture->resp, "n", 1, "v", 1) == dfk_err_nomem);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_NEVER, 0);

  EXPECT(dfk_http_response_set_copy_value(NULL, "n", 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set_copy_value(&fixture->resp, NULL, 1, "v", 1) == dfk_err_badarg);
  EXPECT(dfk_http_response_set_copy_value(&fixture->resp, "n", 1, NULL, 1) == dfk_err_badarg);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_ALWAYS, 0);
  EXPECT(dfk_http_response_set_copy_value(&fixture->resp, "n", 1, "v", 1) == dfk_err_nomem);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_NEVER, 0);
}


TEST_F(fixture, http_response, set_tricky_out_of_memory)
{
  size_t nbytes = DFK_ARENA_SEGMENT_SIZE * 0.4;
  char* buf = malloc(nbytes);
  EXPECT(buf);
  memset(buf, 0, nbytes);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_NTH_FAIL, 1);
  EXPECT_OK(dfk_http_response_set_copy(&fixture->resp, buf, nbytes, buf, nbytes));
  buf[0] = 1;
  EXPECT(dfk_http_response_set_copy(&fixture->resp, buf, nbytes, buf, nbytes) == dfk_err_nomem);
  ut_simulate_out_of_memory(&fixture->dfk, UT_OOM_NEVER, 0);
  free(buf);
}


TEST_F(fixture, http_response, write)
{
  char buf[] = "Hello world";
  fixture->resp.content_length = sizeof(buf) - 1;
  dfk_http_response_write(&fixture->resp, buf, sizeof(buf) - 1);
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Content-Length: 11\r\n"
      "\r\n"
      "Hello world");
}


TEST_F(fixture, http_response, writev)
{
  char buf[] = "Hello world";
  dfk_iovec_t iov[2] = {{buf, 6}, {buf + 6, 5}};
  fixture->resp.content_length = sizeof(buf) - 1;
  dfk_http_response_writev(&fixture->resp, iov, DFK_SIZE(iov));
  expect_resp(&fixture->resp,
      "HTTP/1.0 200 OK\r\n"
      "Content-Length: 11\r\n"
      "\r\n"
      "Hello world");
}

#endif /* DFK_MOCKS */

