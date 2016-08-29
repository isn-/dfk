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
 * The following tests are using dfk_http_request._sock mock,
 * so if DFK_MOCKS are disabled, these tests does not make sense
 */
#if DFK_MOCKS


typedef struct fixture_t {
  dfk_t dfk;
  dfk_arena_t conn_arena;
  dfk_arena_t req_arena;
  dfk_tcp_socket_t sock;
  dfk_http_request_t req;
  dfk_sponge_t reqbuf;
} fixture_t;


static void fixture_setup(fixture_t* f)
{
  EXPECT_OK(dfk_init(&f->dfk));
  EXPECT_OK(dfk_arena_init(&f->conn_arena, &f->dfk));
  EXPECT_OK(dfk_arena_init(&f->req_arena, &f->dfk));
  EXPECT_OK(dfk_http_request_init(&f->req, &f->dfk, &f->req_arena,
                                  &f->conn_arena, &f->sock));
  dfk_sponge_init(&f->reqbuf, &f->dfk);
  f->req._sock_mocked = 1;
  f->req._sock_mock = &f->reqbuf;
}


static void fixture_teardown(fixture_t* f)
{
  dfk_sponge_free(&f->reqbuf);
  EXPECT_OK(dfk_http_request_free(&f->req));
  EXPECT_OK(dfk_arena_free(&f->conn_arena));
  EXPECT_OK(dfk_arena_free(&f->req_arena));
  EXPECT_OK(dfk_free(&f->dfk));
}


typedef ssize_t(*dfk_read_f)(void*, char*, size_t);
static ssize_t readall(dfk_read_f readfunc, void* readobj, char* buf, size_t size)
{
  ssize_t total = 0;
  ssize_t res = readfunc(readobj, buf, size);
  while (res > 0) {
    total += res;
    buf += res;
    size -= res;
    if (!size) {
      break;
    }
    res = readfunc(readobj, buf, size);
  }
  return size ? res : total;
}


TEST_F(fixture, http_request, trivial)
{
  char request[] = "GET / HTTP/1.0\r\n\r\n";
  dfk_sponge_write(&fixture->reqbuf, request, sizeof(request) - 1);
  EXPECT_OK(dfk_http_request_prepare(&fixture->req));
  EXPECT(fixture->req.content_length == 0);
  EXPECT(fixture->req.method == DFK_HTTP_GET);
  EXPECT_BUFSTREQ(fixture->req.url, "/");
  EXPECT(fixture->req.major_version == 1);
  EXPECT(fixture->req.minor_version == 0);
}


TEST_F(fixture, http_request, trivial_chunked_body)
{
  char request[] = "POST / HTTP/1.0\r\n"
                   "Transfer-Encoding: chunked\r\n"
                   "\r\n"
                   "5\r\n"
                   "Hello\r\n"
                   "7\r\n"
                   ", world\r\n"
                   "0\r\n";
  dfk_sponge_write(&fixture->reqbuf, request, sizeof(request) - 1);
  EXPECT_OK(dfk_http_request_prepare(&fixture->req));
  EXPECT(fixture->req.content_length == (uint64_t) -1);
  EXPECT(fixture->req.method == DFK_HTTP_POST);
  EXPECT_BUFSTREQ(fixture->req.url, "/");
  EXPECT(fixture->req.major_version == 1);
  EXPECT(fixture->req.minor_version == 0);
  char buf[12] = {0};
  EXPECT(readall((dfk_read_f) dfk_http_request_read, &fixture->req, buf, sizeof(buf)) == 12);
  EXPECT(!strncmp(buf, "Hello, world", sizeof(buf)));
}


#endif /* DFK_MOCKS */

