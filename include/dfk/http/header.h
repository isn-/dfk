/**
 * @file dfk/http/constants.h
 * Constants for HTTP methods and return codes
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
#include <stddef.h>
#include <dfk/core.h>
#include <dfk/internal/avltree.h>


typedef struct dfk_http_header_t {
  /** @private */
  dfk_avltree_hook_t _hook;
  dfk_buf_t name;
  dfk_buf_t value;
} dfk_http_header_t;


typedef struct dfk_http_header_it {
  /** @private */
  dfk_avltree_it_t _it;
  dfk_http_header_t* header;
} dfk_http_header_it;


/** @internal */
void dfk__http_header_init(dfk_http_header_t* header);


/**
 * Used by dfk_http_request_headers_get and dfk_http_response_headers_get
 * @internal
 */
dfk_buf_t dfk__http_headers_get(dfk_avltree_t* cont, const char* name, size_t namesize);


/**
 * Used by dfk_http_request_headers_begin and dfk_http_response_headers_begin
 * @internal
 */
int dfk__http_headers_begin(dfk_avltree_t* cont, dfk_http_header_it* it);


/**
 * Compare-function for dfk_avltree_t initialization
 * @internal
 */
int dfk__http_headers_cmp(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r);


/**
 * Increment iterator
 *
 * Do not forget to call dfk_http_headers_valid before accessing iterator's value
 */
int dfk_http_headers_next(dfk_http_header_it* it);


/**
 * Check whether iterator's value can be accessed.
 * @returns
 * @li dfk_err_ok if iterator is valid
 * @li dfk_err_eof if iterator is not valid
 * @li dfk_err_badarg if @p it is NULL
 */
int dfk_http_headers_valid(dfk_http_header_it* it);

