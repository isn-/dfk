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
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <curl/curl.h>
#include <dfk.h>
#include <dfk/internal.h>
#include <ut.h>


#define MiB (1024 * 1024)


typedef struct http_fixture_t {
  CURL* curl;
  dfk_t dfk;
  dfk_http_t http;
  dfk_http_handler handler;
  pthread_t dfkthread;
  pthread_mutex_t handler_m;
} http_fixture_t;


static int http_fixture_dyn_handler(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  http_fixture_t* fixture = (http_fixture_t*) http->user.data;
  pthread_mutex_lock(&fixture->handler_m);
  dfk_http_handler handler = fixture->handler;
  pthread_mutex_unlock(&fixture->handler_m);
  if (handler) {
    return handler(http, req, resp);
  } else {
    resp->keepalive = 0;
    return dfk_err_ok;
  }
}


static void http_fixture_set_handler(http_fixture_t* fixture, dfk_http_handler handler)
{
  pthread_mutex_lock(&fixture->handler_m);
  fixture->handler = handler;
  pthread_mutex_unlock(&fixture->handler_m);
}


static void http_fixture_dfkmain(dfk_coro_t* coro, void* p)
{
  http_fixture_t* f = (http_fixture_t*) p;
  DFK_UNUSED(coro);
  dfk_http_init(&f->http, &f->dfk);
  dfk_http_serve(&f->http, "127.0.0.1", 10000, http_fixture_dyn_handler);
  dfk_http_free(&f->http);
}


static void* http_fixture_server_start(void* p)
{
  http_fixture_t* f = (http_fixture_t*) p;
  dfk_run(&f->dfk, http_fixture_dfkmain, p, 0);
  dfk_work(&f->dfk);
  pthread_exit(NULL);
}


static void http_fixture_setup(http_fixture_t* f)
{
  f->curl = curl_easy_init();
  curl_easy_setopt(f->curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(f->curl, CURLOPT_USERAGENT, "dfk-libcurl");
  EXPECT(f->curl);
  dfk_init(&f->dfk);
  f->http.user.data = f;
  f->handler = NULL;
  pthread_mutex_init(&f->handler_m, NULL);
  pthread_create(&f->dfkthread, NULL, &http_fixture_server_start, f);
  {
    int server_ready = 0;
    int nattempt;
    for (nattempt = 0; !server_ready && nattempt < 10; ++nattempt) {
      {
        struct timespec req, rem;
        memset(&req, 0, sizeof(req));
        memset(&rem, 0, sizeof(rem));
        req.tv_nsec = nattempt * nattempt * 1000000;
        nanosleep(&req, &rem);
      }
      CURL* testcurl = curl_easy_init();
      CURLcode res;
      long http_code = 0;
      curl_easy_setopt(testcurl, CURLOPT_URL, "http://127.0.0.1:10000/");
      res = curl_easy_perform(testcurl);
      curl_easy_getinfo(testcurl, CURLINFO_RESPONSE_CODE, &http_code);
      if (res == CURLE_OK && http_code == 200) {
        server_ready = 1;
      }
      curl_easy_cleanup(testcurl);
    }
    EXPECT(server_ready);
  }
}


static void http_fixture_teardown(http_fixture_t* f)
{
  curl_easy_cleanup(f->curl);
  dfk_stop(&f->dfk);
  pthread_join(f->dfkthread, NULL);
  pthread_mutex_destroy(&f->handler_m);
  dfk_free(&f->dfk);
}


static int ut_errors_handler(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  DFK_UNUSED(req);
  resp->code = 200;
  resp->keepalive = 0;
  return 0;
}


static void ut_errors(dfk_coro_t* coro, void* arg)
{
  dfk_http_t http;
  DFK_UNUSED(arg);
  EXPECT(dfk_http_init(NULL, NULL) == dfk_err_badarg);
  EXPECT(dfk_http_init(&http, NULL) == dfk_err_badarg);
  EXPECT(dfk_http_init(NULL, coro->dfk) == dfk_err_badarg);
  EXPECT_OK(dfk_http_init(&http, coro->dfk));
  EXPECT(dfk_http_free(NULL) == dfk_err_badarg);
  EXPECT(dfk_http_stop(NULL) == dfk_err_badarg);
  EXPECT(dfk_http_serve(NULL, NULL, 0, NULL) == dfk_err_badarg);
  EXPECT(dfk_http_serve(&http, NULL, 0, NULL) == dfk_err_badarg);
  EXPECT(dfk_http_serve(NULL, "127.0.0.1", 0, NULL) == dfk_err_badarg);
  EXPECT(dfk_http_serve(NULL, NULL, 0, ut_errors_handler) == dfk_err_badarg);
  EXPECT(dfk_http_serve(&http, "127.0.0.1", 10000, NULL) == dfk_err_badarg);
  EXPECT_OK(dfk_http_free(&http));
}


TEST(http, errors)
{
  dfk_t dfk;
  EXPECT_OK(dfk_init(&dfk));
  EXPECT(dfk_run(&dfk, ut_errors, NULL, 0));
  EXPECT_OK(dfk_work(&dfk));
  EXPECT_OK(dfk_free(&dfk));
}


TEST_F(http_fixture, http, get_no_url_no_content)
{
  CURLcode res;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_parse_common_headers(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  EXPECT(req->method == DFK_HTTP_HEAD);
  EXPECT_BUFSTREQ(req->url, "/");
  EXPECT_BUFSTREQ(req->user_agent, "dfk-libcurl");
  EXPECT_BUFSTREQ(req->host, "127.0.0.1:10000");
  EXPECT_BUFSTREQ(req->accept, "*/*");
  EXPECT(!req->content_type.data);
  EXPECT(!req->content_type.size);
  EXPECT(req->content_length == 0);
  resp->code = 200;
  resp->keepalive = 0;
  return 0;
}


TEST_F(http_fixture, http, parse_common_headers)
{
  CURLcode res;
  http_fixture_set_handler(fixture, ut_parse_common_headers);
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_NOBODY, 1L);
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_iterate_headers(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  EXPECT(req->method == DFK_HTTP_POST);
  EXPECT_BUFSTREQ(req->url, "/");
  EXPECT_BUFSTREQ(req->user_agent, "dfk-libcurl");
  EXPECT_BUFSTREQ(req->host, "127.0.0.1:10000");
  EXPECT_BUFSTREQ(req->accept, "*/*");
  EXPECT_BUFSTREQ(req->content_type, "application/x-www-form-urlencoded");
  EXPECT(req->content_length == 0);
  {
    char* expected_fields[] = {
      "User-Agent",
      "Host",
      "Expect",
      "Content-Type",
      "Accept"
    };
    char* expected_values[] = {
      "dfk-libcurl",
      "127.0.0.1:10000",
      "100-continue",
      "application/x-www-form-urlencoded",
      "*/*"
    };
    dfk_strmap_it it;
    size_t i = 0;
    assert(DFK_SIZE(expected_fields) == DFK_SIZE(expected_values));
    dfk_strmap_begin(&req->headers, &it);
    while (dfk_strmap_it_valid(&it) == dfk_err_ok) {
      EXPECT_BUFSTREQ(it.item->key, expected_fields[i]);
      EXPECT_BUFSTREQ(it.item->value, expected_values[i]);
      dfk_strmap_it_next(&it);
      ++i;
    }
    EXPECT(i == DFK_SIZE(expected_fields));
  }
  resp->code = 200;
  resp->keepalive = 0;
  return 0;
}


TEST_F(http_fixture, http, iterate_headers)
{
  CURLcode res;
  http_fixture_set_handler(fixture, ut_iterate_headers);
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_POST, 1L);
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


typedef struct ut_curl_postdata_t {
  dfk_buf_t buf;
  size_t nrepeat;
  size_t nread;
} ut_curl_postdata_t;


static void ut_curl_postdata_init(ut_curl_postdata_t* pd)
{
  assert(pd);
  pd->buf = (dfk_buf_t) {NULL, 0};
  pd->nrepeat = 1;
  pd->nread = 0;
}


static size_t ut_read_callback(void* buffer, size_t size, size_t nitems, void* ud)
{
  if (size * nitems < 1) {
    return 0;
  }

  ut_curl_postdata_t* pd = (ut_curl_postdata_t*) ud;
  char* out = buffer;

  assert(pd->nread <= pd->nrepeat * pd->buf.size);
  size_t toread = DFK_MIN(size * nitems, pd->nrepeat * pd->buf.size - pd->nread);
  size_t res = toread;
  {
    size_t padsize = (pd->buf.size - toread % pd->buf.size) % pd->buf.size;
    memcpy(out, pd->buf.data + pd->nread % pd->buf.size, padsize);
    toread -= padsize;
    out += padsize;
  }

  while (toread >= pd->buf.size) {
    memcpy(out, pd->buf.data, pd->buf.size);
    toread -= pd->buf.size;
    out += pd->buf.size;
  }

  if (toread) {
    assert(toread < pd->buf.size);
    memcpy(out, pd->buf.data, toread);
  }
  pd->nread += res;
  return res;
}


static int ut_content_length(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  EXPECT(req->content_length == 9);
  char buf[9] = {0};
  size_t nread = dfk_http_request_read(req, buf, req->content_length);
  EXPECT(nread == req->content_length);
  resp->code = 200;
  resp->keepalive = 0;
  return 0;
}


TEST_F(http_fixture, http, content_length)
{
  http_fixture_set_handler(fixture, ut_content_length);
  ut_curl_postdata_t pd;
  ut_curl_postdata_init(&pd);
  pd.buf = (dfk_buf_t) {"some data", 9};
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(fixture->curl, CURLOPT_READFUNCTION, ut_read_callback);
  curl_easy_setopt(fixture->curl, CURLOPT_READDATA, &pd);
  curl_easy_setopt(fixture->curl, CURLOPT_POSTFIELDSIZE, pd.buf.size);
  CURLcode res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_post_9_bytes(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  EXPECT(req->content_length == 9);
  char buf[9] = {0};
  size_t nread = dfk_http_request_read(req, buf, req->content_length);
  EXPECT(nread == req->content_length);
  EXPECT(memcmp(buf, "some data", 9) == 0);
  resp->code = 200;
  resp->keepalive = 0;
  return 0;
}


TEST_F(http_fixture, http, post_9_bytes)
{
  http_fixture_set_handler(fixture, ut_post_9_bytes);
  ut_curl_postdata_t pd;
  ut_curl_postdata_init(&pd);
  pd.buf = (dfk_buf_t) {"some data", 9};
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(fixture->curl, CURLOPT_READFUNCTION, ut_read_callback);
  curl_easy_setopt(fixture->curl, CURLOPT_READDATA, &pd);
  curl_easy_setopt(fixture->curl, CURLOPT_POSTFIELDSIZE, pd.buf.size);
  CURLcode res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_post_10_mb(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  EXPECT(req->content_length == 10 * MiB);
  size_t bufsize = MiB;
  char* buf = DFK_MALLOC(http->dfk, bufsize);
  assert(buf);
  size_t totalread = 0;
  while (totalread != 10 * MiB) {
    size_t nread = dfk_http_request_read(req, buf, bufsize);
    EXPECT(nread > 0);
    totalread += nread;
  }
  DFK_FREE(http->dfk, buf);
  resp->code = 200;
  resp->keepalive = 0;
  return 0;
}


TEST_F(http_fixture, http, post_10_mb)
{
  http_fixture_set_handler(fixture, ut_post_10_mb);
  ut_curl_postdata_t pd;
  ut_curl_postdata_init(&pd);
  pd.buf = (dfk_buf_t) {"somedata", 8};
  pd.nrepeat = 10 * MiB / pd.buf.size;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(fixture->curl, CURLOPT_READFUNCTION, ut_read_callback);
  curl_easy_setopt(fixture->curl, CURLOPT_READDATA, &pd);
  curl_easy_setopt(fixture->curl, CURLOPT_POSTFIELDSIZE, 10 * MiB);
  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "Expect:");
  EXPECT(curl_easy_setopt(fixture->curl, CURLOPT_HTTPHEADER, chunk) == CURLE_OK);
  CURLcode res = curl_easy_perform(fixture->curl);
  curl_slist_free_all(chunk);
  EXPECT(res == CURLE_OK);
}


static int ut_output_headers(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  DFK_UNUSED(req);
  dfk_strmap_item_t* i = dfk_strmap_item_acopy(req->_request_arena, "Server", 6, "rocks", 5);
  dfk_strmap_insert(&resp->headers, i);
  i = dfk_strmap_item_acopy(req->_request_arena, "Foo", 3, "bar", 3);
  dfk_strmap_insert(&resp->headers, i);
  resp->code = 200;
  resp->keepalive = 0;
  return 0;
}


static size_t ut_output_headers_callback(char* buffer, size_t size, size_t nitems, void* userdata)
{
  int* counter = (int*) userdata;
  dfk_buf_t buf = {buffer, size * nitems};
  if (*counter == 1) {
    EXPECT_BUFSTREQ(buf, "Server: rocks\r\n");
  } else if (*counter == 2) {
    EXPECT_BUFSTREQ(buf, "Foo: bar\r\n");
  }
  (*counter)++;
  return size * nitems;
}


TEST_F(http_fixture, http, output_headers)
{
  CURLcode res;
  http_fixture_set_handler(fixture, ut_output_headers);
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_HEADERFUNCTION, ut_output_headers_callback);
  int counter = 0;
  curl_easy_setopt(fixture->curl, CURLOPT_HEADERDATA, &counter);
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static void ut_stop_during_request_stopper(dfk_coro_t* coro, void* arg)
{
  DFK_UNUSED(coro);
  dfk_http_t* http = (dfk_http_t*) arg;
  dfk_http_stop(http);
}


static int ut_stop_during_request(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  DFK_UNUSED(req);
  dfk_run(http->dfk, ut_stop_during_request_stopper, http, 0);
  DFK_POSTPONE(http->dfk);
  resp->code = 200;
  resp->keepalive = 0;
  return dfk_err_ok;
}


TEST_F(http_fixture, http, stop_during_request)
{
  CURLcode res;
  http_fixture_set_handler(fixture, ut_stop_during_request);
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_keepalive_some_requests_nsockets = 0;


static curl_socket_t ut_keepalive_some_requests_opensocket_callback(
    void* client, curlsocktype purpose, struct curl_sockaddr* addr)
{
  DFK_UNUSED(client);
  DFK_UNUSED(purpose);
  ++ut_keepalive_some_requests_nsockets;
  return socket(addr->family, addr->socktype, addr->protocol);
}


static int ut_keepalive_some_requests(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  DFK_UNUSED(http);
  DFK_UNUSED(req);
  resp->code = 200;
  return dfk_err_ok;
}


TEST_F(http_fixture, http, keepalive_some_requests)
{
  CURLcode res;
  size_t nrequests = 2;
  fixture->http.keepalive_requests = nrequests;
  http_fixture_set_handler(fixture, ut_keepalive_some_requests);
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_OPENSOCKETFUNCTION, ut_keepalive_some_requests_opensocket_callback);
  for (size_t i = 0; i < nrequests; ++i) {
    res = curl_easy_perform(fixture->curl);
    EXPECT(res == CURLE_OK);
  }
  EXPECT(ut_keepalive_some_requests_nsockets == 1);
}

