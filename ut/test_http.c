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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <curl/curl.h>
#include <dfk.h>
#include <dfk/internal.h>
#include "ut.h"


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
  curl_easy_setopt(fixture->curl, CURLOPT_NOBODY, 1);
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
      ASSERT_BUFSTREQ_RET(it.value.field, expected_fields[i], 0);
      ASSERT_BUFSTREQ_RET(it.value.value, expected_values[i], 0);
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
  curl_easy_setopt(fixture->curl, CURLOPT_POST, 1);
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}

