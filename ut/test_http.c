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
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <curl/curl.h>
#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/http.h>
#include "ut.h"


#define MiB (1024 * 1024)


typedef struct http_fixture_t {
  CURL* curl;
  dfk_t dfk;
  dfk_http_t http;
  dfk_http_handler handler;
  pthread_t dfkthread;
} http_fixture_t;


static int http_fixture_dyn_handler(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  http_fixture_t* fixture = (http_fixture_t*) http->user.data;
  if (fixture->handler) {
    return fixture->handler(http, req, resp);
  } else {
    resp->code = 200;
    return 0;
  }
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
  }
}


static void http_fixture_teardown(http_fixture_t* f)
{
  curl_easy_cleanup(f->curl);
  dfk_stop(&f->dfk);
  pthread_join(f->dfkthread, NULL);
}


static int ut_errors_handler(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  DFK_UNUSED(req);
  resp->code = 200;
  return 0;
}


static void ut_errors(dfk_coro_t* coro, void* arg)
{
  dfk_http_t http;
  DFK_UNUSED(arg);
  ASSERT(dfk_http_init(NULL, NULL) == dfk_err_badarg);
  ASSERT(dfk_http_init(&http, NULL) == dfk_err_badarg);
  ASSERT(dfk_http_init(NULL, coro->dfk) == dfk_err_badarg);
  ASSERT_OK(dfk_http_init(&http, coro->dfk));
  ASSERT(dfk_http_free(NULL) == dfk_err_badarg);
  ASSERT(dfk_http_stop(NULL) == dfk_err_badarg);
  ASSERT(dfk_http_serve(NULL, NULL, 0, NULL) == dfk_err_badarg);
  ASSERT(dfk_http_serve(&http, NULL, 0, NULL) == dfk_err_badarg);
  ASSERT(dfk_http_serve(NULL, "127.0.0.1", 0, NULL) == dfk_err_badarg);
  ASSERT(dfk_http_serve(NULL, NULL, 0, ut_errors_handler) == dfk_err_badarg);
  ASSERT(dfk_http_serve(&http, "127.0.0.1", 10000, NULL) == dfk_err_badarg);
  ASSERT_OK(dfk_http_free(&http));
}


TEST(http, errors)
{
  dfk_t dfk;
  ASSERT_OK(dfk_init(&dfk));
  ASSERT(dfk_run(&dfk, ut_errors, NULL, 0));
  ASSERT_OK(dfk_work(&dfk));
  ASSERT_OK(dfk_free(&dfk));
}


TEST(http, reason_phrase)
{
  /* For the most common error codes, check exact phrase */
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_OK), "OK"));
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_CREATED), "Created"));
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_BAD_REQUEST), "Bad Request"));
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_NOT_FOUND), "Not Found"));
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_REQUEST_TIMEOUT), "Request Timeout"));
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_INTERNAL_SERVER_ERROR), "Internal Server Error"));
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_BAD_GATEWAY), "Bad Gateway"));
  EXPECT(!strcmp(dfk__http_reason_phrase(DFK_HTTP_GATEWAY_TIMEOUT), "Gateway Timeout"));

  /* For other codes, check presence of the phrase */
  dfk_http_status_e st[] = {
    DFK_HTTP_CONTINUE, DFK_HTTP_SWITCHING_PROTOCOLS, DFK_HTTP_PROCESSING, DFK_HTTP_OK,
    DFK_HTTP_CREATED, DFK_HTTP_ACCEPTED, DFK_HTTP_NON_AUTHORITATIVE_INFORMATION,
    DFK_HTTP_NO_CONTENT, DFK_HTTP_RESET_CONTENT, DFK_HTTP_PARTIAL_CONTENT, DFK_HTTP_MULTI_STATUS,
    DFK_HTTP_ALREADY_REPORTED, DFK_HTTP_IM_USED, DFK_HTTP_MULTIPLE_CHOICES,
    DFK_HTTP_MOVED_PERMANENTLY, DFK_HTTP_FOUND, DFK_HTTP_SEE_OTHER, DFK_HTTP_NOT_MODIFIED,
    DFK_HTTP_USE_PROXY, DFK_HTTP_SWITCH_PROXY, DFK_HTTP_TEMPORARY_REDIRECT,
    DFK_HTTP_PERMANENT_REDIRECT, DFK_HTTP_BAD_REQUEST, DFK_HTTP_UNAUTHORIZED,
    DFK_HTTP_PAYMENT_REQUIRED, DFK_HTTP_FORBIDDEN, DFK_HTTP_NOT_FOUND,
    DFK_HTTP_METHOD_NOT_ALLOWED, DFK_HTTP_NOT_ACCEPTABLE, DFK_HTTP_PROXY_AUTHENTICATION_REQUIRED,
    DFK_HTTP_REQUEST_TIMEOUT, DFK_HTTP_CONFLICT, DFK_HTTP_GONE, DFK_HTTP_LENGTH_REQUIRED,
    DFK_HTTP_PRECONDITION_FAILED, DFK_HTTP_PAYLOAD_TOO_LARGE, DFK_HTTP_URI_TOO_LONG,
    DFK_HTTP_UNSUPPORTED_MEDIA_TYPE, DFK_HTTP_RANGE_NOT_SATISFIABLE, DFK_HTTP_EXPECTATION_FAILED,
    DFK_HTTP_I_AM_A_TEAPOT, DFK_HTTP_MISDIRECTED_REQUEST, DFK_HTTP_UNPROCESSABLE_ENTITY,
    DFK_HTTP_LOCKED, DFK_HTTP_FAILED_DEPENDENCY, DFK_HTTP_UPGRADE_REQUIRED,
    DFK_HTTP_PRECONDITION_REQUIRED, DFK_HTTP_TOO_MANY_REQUESTS,
    DFK_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE, DFK_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS,
    DFK_HTTP_INTERNAL_SERVER_ERROR, DFK_HTTP_NOT_IMPLEMENTED, DFK_HTTP_BAD_GATEWAY,
    DFK_HTTP_SERVICE_UNAVAILABLE, DFK_HTTP_GATEWAY_TIMEOUT, DFK_HTTP_HTTP_VERSION_NOT_SUPPORTED,
    DFK_HTTP_VARIANT_ALSO_NEGOTIATES, DFK_HTTP_INSUFFICIENT_STORAGE, DFK_HTTP_LOOP_DETECTED,
    DFK_HTTP_NOT_EXTENDED, DFK_HTTP_NETWORK_AUTHENTICATION_REQUIRED
  };

  for (size_t i = 0; i < DFK_SIZE(st); ++i) {
    const char* rp = dfk__http_reason_phrase(st[i]);
    EXPECT(rp);
    EXPECT(rp[0] != '\0');
  }
}


TEST(http, reason_phrase_unknown)
{
  EXPECT(!strcmp(dfk__http_reason_phrase(0), "Unknown"));
}


TEST_F(http_fixture, http, get_no_url_no_content)
{
  CURLcode res;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_parse_common_headers(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  ASSERT_RET(req->method == DFK_HTTP_HEAD, 0);
  ASSERT_BUFSTREQ_RET(req->url, "/", 0);
  ASSERT_BUFSTREQ_RET(req->user_agent, "dfk-libcurl", 0);
  ASSERT_BUFSTREQ_RET(req->host, "127.0.0.1:10000", 0);
  ASSERT_BUFSTREQ_RET(req->accept, "*/*", 0);
  ASSERT_RET(!req->content_type.data, 0);
  ASSERT_RET(!req->content_type.size, 0);
  ASSERT_RET(req->content_length == 0, 0);
  resp->code = 200;
  return 0;
}


TEST_F(http_fixture, http, parse_common_headers)
{
  CURLcode res;
  fixture->handler = ut_parse_common_headers;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_NOBODY, 1L);
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_iterate_headers(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  ASSERT_RET(req->method == DFK_HTTP_POST, 0);
  ASSERT_BUFSTREQ_RET(req->url, "/", 0);
  ASSERT_BUFSTREQ_RET(req->user_agent, "dfk-libcurl", 0);
  ASSERT_BUFSTREQ_RET(req->host, "127.0.0.1:10000", 0);
  ASSERT_BUFSTREQ_RET(req->accept, "*/*", 0);
  ASSERT_BUFSTREQ_RET(req->content_type, "application/x-www-form-urlencoded", 0);
  ASSERT_RET(req->content_length == 0, 0);
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
    dfk_http_headers_it it;
    size_t i = 0;
    assert(DFK_SIZE(expected_fields) == DFK_SIZE(expected_values));
    dfk_http_headers_begin(req, &it);
    while (dfk_http_headers_valid(&it) == dfk_err_ok) {
      ASSERT_BUFSTREQ_RET(it.field, expected_fields[i], 0);
      ASSERT_BUFSTREQ_RET(it.value, expected_values[i], 0);
      dfk_http_headers_next(&it);
      ++i;
    }
    ASSERT_RET(i == DFK_SIZE(expected_fields), 0);
  }
  resp->code = 200;
  return 0;
}


TEST_F(http_fixture, http, iterate_headers)
{
  CURLcode res;
  fixture->handler = ut_iterate_headers;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_POST, 1L);
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}


static int ut_request_errors(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  DFK_UNUSED(req);
  EXPECT(dfk_http_get(NULL, "foo", 3).data == NULL);
  EXPECT(dfk_http_get(NULL, "foo", 3).size == 0);
  EXPECT(dfk_http_get(req, NULL, 3).data == NULL);
  EXPECT(dfk_http_get(req, NULL, 3).size == 0);
  EXPECT(dfk_http_get(req, "foo", 0).data == NULL);
  EXPECT(dfk_http_get(req, "foo", 0).size == 0);
  {
    dfk_http_headers_it it;
    EXPECT(dfk_http_headers_begin(NULL, &it) == dfk_err_badarg);
    EXPECT(dfk_http_headers_begin(req, NULL) == dfk_err_badarg);
    EXPECT(dfk_http_headers_next(NULL) == dfk_err_badarg);
    EXPECT(dfk_http_headers_valid(NULL) == dfk_err_badarg);
  }
  {
    dfk_http_headers_it it;
    EXPECT_OK(dfk_http_headers_begin(req, &it));
    while (dfk_http_headers_valid(&it) == dfk_err_ok) {
      EXPECT_OK(dfk_http_headers_next(&it));
    }
    EXPECT(dfk_http_headers_next(&it) == dfk_err_eof);
  }
  resp->code = 200;
  return 0;
}


TEST_F(http_fixture, http, request_errors)
{
  CURLcode res;
  fixture->handler = ut_request_errors;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
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


static int ut_content_length(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  ASSERT_RET(req->content_length == 9, 0);
  resp->code = 200;
  return 0;
}


TEST_F(http_fixture, http, content_length)
{
  fixture->handler = ut_content_length;
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


static int ut_post_9_bytes(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  ASSERT_RET(req->content_length == 9, 0);
  char buf[9] = {0};
  size_t nread = dfk_http_read(req, buf, req->content_length);
  ASSERT_RET(nread == req->content_length, 0);
  ASSERT_RET(memcmp(buf, "some data", 9) == 0, 0);
  resp->code = 200;
  return 0;
}


TEST_F(http_fixture, http, post_9_bytes)
{
  fixture->handler = ut_post_9_bytes;
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


static int ut_post_100_mb(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  ASSERT_RET(req->content_length == 100 * MiB, 0);
  size_t bufsize = MiB;
  char* buf = DFK_MALLOC(http->dfk, bufsize);
  assert(buf);
  size_t totalread = 0;
  while (totalread != 100 * MiB) {
    size_t nread = dfk_http_read(req, buf, bufsize);
    ASSERT_RET(nread > 0, 0);
    totalread += nread;
  }
  DFK_FREE(http->dfk, buf);
  resp->code = 200;
  return 0;
}


TEST_F(http_fixture, http, post_100_mb)
{
  fixture->handler = ut_post_100_mb;
  ut_curl_postdata_t pd;
  ut_curl_postdata_init(&pd);
  pd.buf = (dfk_buf_t) {"somedata", 8};
  pd.nrepeat = 100 * MiB / pd.buf.size;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(fixture->curl, CURLOPT_READFUNCTION, ut_read_callback);
  curl_easy_setopt(fixture->curl, CURLOPT_READDATA, &pd);
  curl_easy_setopt(fixture->curl, CURLOPT_POSTFIELDSIZE, 100 * MiB);
  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "Expect:");
  ASSERT(curl_easy_setopt(fixture->curl, CURLOPT_HTTPHEADER, chunk) == CURLE_OK);
  CURLcode res = curl_easy_perform(fixture->curl);
  curl_slist_free_all(chunk);
  EXPECT(res == CURLE_OK);
}


static int ut_output_headers(dfk_http_t* http, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(http);
  DFK_UNUSED(req);
  dfk_http_set(resp, "Server", 6, "rocks", 5);
  dfk_http_set(resp, "Foo", 3, "bar", 3);
  resp->code = 200;
  return 0;
}


static size_t ut_output_headers_callback(char* buffer, size_t size, size_t nitems, void* userdata)
{
  int* counter = (int*) userdata;
  ASSERT_RET(0 <= *counter && *counter <= 2, 0);
  dfk_buf_t buf = {buffer, size * nitems};
  if (*counter == 1) {
    ASSERT_BUFSTREQ_RET(buf, "Server: rocks\r\n", 0);
  } else if (*counter == 2) {
    ASSERT_BUFSTREQ_RET(buf, "Foo: bar\r\n", 0);
  }
  (*counter)++;
  return size * nitems;
}


TEST_F(http_fixture, http, output_headers)
{
  CURLcode res;
  fixture->handler = ut_output_headers;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  curl_easy_setopt(fixture->curl, CURLOPT_HEADERFUNCTION, ut_output_headers_callback);
  int counter = 0;
  curl_easy_setopt(fixture->curl, CURLOPT_HEADERDATA, &counter);
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}

