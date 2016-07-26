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

#include <assert.h>
#include <dfk/core.h>
#include <dfk/internal.h>
#include <dfk/http/header.h>


void dfk__http_header_init(dfk_http_header_t* header)
{
  assert(header);
  dfk_avltree_hook_init(&header->_hook);
  header->name = (dfk_buf_t) {NULL, 0};
  header->value = (dfk_buf_t) {NULL, 0};
}


static int dfk__http_header_lookup_cmp(dfk_avltree_hook_t* l, void* r)
{
  assert(l);
  assert(r);
  dfk_http_header_t* lh = (dfk_http_header_t*) l;
  dfk_buf_t* rh = (dfk_buf_t*) r;
  size_t tocmp = DFK_MIN(lh->name.size, rh->size);
  int res = strncmp(lh->name.data, rh->data, tocmp);
  if (res) {
    return res;
  } else {
    return lh->name.size > rh->size;
  }
  return res;
}


int dfk__http_headers_cmp(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r)
{
  assert(l);
  assert(r);
  return dfk__http_header_lookup_cmp(l, &((dfk_http_header_t*) r)->name);
}


dfk_buf_t dfk__http_headers_get(dfk_avltree_t* cont, const char* name, size_t namesize)
{
  assert(cont);
  assert(name || !namesize);
  dfk_buf_t e = (dfk_buf_t) {(char*) name, namesize};
  dfk_http_header_t* h = (dfk_http_header_t*)
    dfk_avltree_lookup(cont, &e, dfk__http_header_lookup_cmp);
  if (h) {
    return h->value;
  } else {
    return (dfk_buf_t) {NULL, 0};
  }
}


int dfk__http_headers_begin(dfk_avltree_t* cont, dfk_http_header_it* it)
{
  assert(cont);
  assert(it);
  dfk_avltree_it_init(cont, &it->_it);
  it->header = (dfk_http_header_t*) it->_it.value;
  return dfk_err_ok;
}


int dfk_http_headers_next(dfk_http_header_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  if (!dfk_avltree_it_valid(&it->_it)) {
    return dfk_err_eof;
  }
  dfk_avltree_it_next(&it->_it);
  if (dfk_avltree_it_valid(&it->_it)) {
    it->header = (dfk_http_header_t*) it->_it.value;
  }
  return dfk_err_ok;
}


int dfk_http_headers_valid(dfk_http_header_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  return dfk_avltree_it_valid(&it->_it) ? dfk_err_ok : dfk_err_eof;
}


