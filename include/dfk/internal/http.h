/**
 * @file dfk/internal/http.h
 * HTTP server private methods
 *
 * @copyright
 * Copyright (c) 2016, Stanislav Ivochkin. All Rights Reserved.
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

#pragma once
#include <dfk/core.h>
#include <dfk/http.h>


void dfk__http_req_init(dfk_http_req_t* req, dfk_t* dfk, dfk_arena_t* request_arena,
                        dfk_arena_t* connection_arena, dfk_tcp_socket_t* sock);
void dfk__http_req_free(dfk_http_req_t* req);


void dfk__http_resp_init(dfk_http_resp_t* resp, dfk_t* dfk, dfk_arena_t* request_arena,
                         dfk_arena_t* connection_arena, dfk_tcp_socket_t* sock);
void dfk__http_resp_free(dfk_http_resp_t* resp);
int dfk__http_resp_flush_headers(dfk_http_resp_t* resp);
int dfk__http_resp_flush(dfk_http_resp_t* resp);

const char* dfk__http_reason_phrase(dfk_http_status_e status);

