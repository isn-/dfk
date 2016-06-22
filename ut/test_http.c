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
  EXPECT(f->curl);
  dfk_init(&f->dfk);
  f->http.user.data = f;
  f->handler = NULL;
  pthread_create(&f->dfkthread, NULL, &http_fixture_server_start, f);
}


static void http_fixture_teardown(http_fixture_t* f)
{
  curl_easy_cleanup(f->curl);
  dfk_stop(&f->dfk);
  pthread_join(f->dfkthread, NULL);
}


DISABLED_TEST_F(http_fixture, http, get_no_url_no_content)
{
  CURLcode res;
  curl_easy_setopt(fixture->curl, CURLOPT_URL, "http://127.0.0.1:10000/");
  res = curl_easy_perform(fixture->curl);
  EXPECT(res == CURLE_OK);
}

