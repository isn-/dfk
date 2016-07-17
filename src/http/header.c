/**
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


