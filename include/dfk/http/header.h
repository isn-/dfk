/**
 * @file dfk/http/constants.h
 * Constants for HTTP methods and return codes
 *
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

