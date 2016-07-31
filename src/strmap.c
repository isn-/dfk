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
#include <string.h>
#include <dfk/strmap.h>
#include <dfk/internal.h>

static int dfk_strmap_lookup_cmp(dfk_avltree_hook_t* l, void* r)
{
  assert(l);
  assert(r);
  dfk_strmap_item_t* lh = (dfk_strmap_item_t*) l;
  dfk_buf_t* rh = (dfk_buf_t*) r;
  size_t tocmp = DFK_MIN(lh->key.size, rh->size);
  int res = strncmp(lh->key.data, rh->data, tocmp);
  if (res) {
    return res;
  } else {
    return lh->key.size > rh->size;
  }
  return res;
}


static int dfk_strmap_cmp(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r)
{
  assert(l);
  assert(r);
  return dfk_strmap_lookup_cmp(l, &((dfk_strmap_item_t*) r)->key);
}


int dfk_strmap_item_init(dfk_strmap_item_t* item,
                         const char* key, size_t keylen,
                         const char* value, size_t valuelen)
{
  if (!item) {
    return dfk_err_badarg;
  }
  dfk_avltree_hook_init(&item->_hook);
  item->key = (dfk_buf_t) {(char*) key, keylen};
  item->value = (dfk_buf_t) {(char*) value, valuelen};
  return dfk_err_ok;
}


dfk_strmap_item_t* dfk_strmap_item_copy(
    dfk_t* dfk,
    const char* key, size_t keylen,
    const char* value, size_t valuelen)
{
  if (!dfk) {
    return NULL;
  }
  if ((!key && keylen) || (!value && valuelen)) {
    dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  /* Store key, value and item_t in the one memory block, save 2 alloc's */
  char* buf = DFK_MALLOC(dfk, sizeof(dfk_strmap_item_t) + keylen + valuelen);
  if (!buf) {
    dfk->dfk_errno = dfk_err_nomem;
    return NULL;
  }
  char* pkey = buf + sizeof(dfk_strmap_item_t);
  char* pvalue = pkey + keylen;
  memcpy(pkey, key, keylen);
  memcpy(pvalue, value, valuelen);
  int ret = dfk_strmap_item_init((dfk_strmap_item_t*) buf,
                                 pkey, keylen, pvalue, valuelen);
  assert(ret == dfk_err_ok);
  DFK_UNUSED(ret);
  return (dfk_strmap_item_t*) buf;
}


dfk_strmap_item_t* dfk_strmap_item_copy_key(
    dfk_t* dfk,
    const char* key, size_t keylen,
    const char* value, size_t valuelen)
{
  if (!dfk) {
    return NULL;
  }
  if ((!key && keylen) || (!value && valuelen)) {
    dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  /* Store key and item_t in the one memory block, save 1 alloc */
  char* buf = DFK_MALLOC(dfk, sizeof(dfk_strmap_item_t) + keylen);
  if (!buf) {
    dfk->dfk_errno = dfk_err_nomem;
    return NULL;
  }
  char* pkey = buf + sizeof(dfk_strmap_item_t);
  memcpy(pkey, key, keylen);
  int ret = dfk_strmap_item_init((dfk_strmap_item_t*) buf,
                                 pkey, keylen, value, valuelen);
  assert(ret == dfk_err_ok);
  DFK_UNUSED(ret);
  return (dfk_strmap_item_t*) buf;
}


dfk_strmap_item_t* dfk_strmap_item_copy_value(
    dfk_t* dfk,
    const char* key, size_t keylen,
    const char* value, size_t valuelen)
{
  if (!dfk) {
    return NULL;
  }
  if ((!key && keylen) || (!value && valuelen)) {
    dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  /* Store value and item_t in the one memory block, save 1 alloc */
  char* buf = DFK_MALLOC(dfk, sizeof(dfk_strmap_item_t) + valuelen);
  if (!buf) {
    dfk->dfk_errno = dfk_err_nomem;
    return NULL;
  }
  char* pvalue = buf + sizeof(dfk_strmap_item_t);
  memcpy(pvalue, value, valuelen);
  int ret = dfk_strmap_item_init((dfk_strmap_item_t*) buf,
                                 key, keylen, pvalue, valuelen);
  assert(ret == dfk_err_ok);
  DFK_UNUSED(ret);
  return (dfk_strmap_item_t*) buf;
}


dfk_strmap_item_t* dfk_strmap_item_acopy(
    dfk_arena_t* arena,
    const char* key, size_t keylen,
    const char* value, size_t valuelen)
{
  if (!arena) {
    return NULL;
  }
  if ((!key && keylen) || (!value && valuelen)) {
    arena->dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  char* pkey = dfk_arena_alloc_copy(arena, key, keylen);
  if (!pkey) {
    arena->dfk->dfk_errno = dfk_err_nomem;
    return NULL;
  }
  char* pvalue = dfk_arena_alloc_copy(arena, value, valuelen);
  if (!pvalue) {
    arena->dfk->dfk_errno = dfk_err_nomem;
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    arena->dfk->dfk_errno = dfk_err_nomem;
    return NULL;
  }
  int ret = dfk_strmap_item_init(item, pkey, keylen, pvalue, valuelen);
  assert(ret == dfk_err_ok);
  DFK_UNUSED(ret);
  return (dfk_strmap_item_t*) item;
}


dfk_strmap_item_t* dfk_strmap_item_acopy_key(
    dfk_arena_t* arena,
    const char* key, size_t keylen,
    const char* value, size_t valuelen)
{
  if (!arena) {
    return NULL;
  }
  if ((!key && keylen) || (!value && valuelen)) {
    arena->dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  char* pkey = dfk_arena_alloc_copy(arena, key, keylen);
  if (!pkey) {
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return NULL;
  }
  int ret = dfk_strmap_item_init(item, pkey, keylen, value, valuelen);
  assert(ret == dfk_err_ok);
  DFK_UNUSED(ret);
  return (dfk_strmap_item_t*) item;
}


dfk_strmap_item_t* dfk_strmap_item_acopy_value(
    dfk_arena_t* arena,
    const char* key, size_t keylen,
    const char* value, size_t valuelen)
{
  if (!arena) {
    return NULL;
  }
  if ((!key && keylen) || (!value && valuelen)) {
    arena->dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  char* pvalue = dfk_arena_alloc_copy(arena, value, valuelen);
  if (!pvalue) {
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return NULL;
  }
  int ret = dfk_strmap_item_init(item, key, keylen, pvalue, valuelen);
  assert(ret == dfk_err_ok);
  DFK_UNUSED(ret);
  return (dfk_strmap_item_t*) item;
}


int dfk_strmap_item_free(dfk_strmap_item_t* item)
{
  if (!item) {
    return dfk_err_badarg;
  }
  return dfk_err_ok;
}


int dfk_strmap_init(dfk_strmap_t* map)
{
  if (!map) {
    return dfk_err_badarg;
  }
  dfk_avltree_init(&map->_cont, dfk_strmap_cmp);
  return dfk_err_ok;
}


size_t dfk_strmap_size(dfk_strmap_t* map)
{
  if (!map) {
    return (size_t) -1;
  }
  return dfk_avltree_size(&map->_cont);
}


int dfk_strmap_free(dfk_strmap_t* map)
{
  if (!map) {
    return dfk_err_badarg;
  }
  dfk_avltree_free(&map->_cont);
  return dfk_err_ok;
}


dfk_buf_t dfk_strmap_get(dfk_strmap_t* map, const char* key, size_t keylen)
{
  if (!map || (!key && keylen)) {
    return (dfk_buf_t) {NULL, 0};
  }
  dfk_buf_t e = (dfk_buf_t) {(char*) key, keylen};
  dfk_strmap_item_t* i = (dfk_strmap_item_t*)
    dfk_avltree_lookup(&map->_cont, &e, dfk_strmap_lookup_cmp);
  return i ? i->value : (dfk_buf_t) {NULL, 0};
}


int dfk_strmap_erase(dfk_strmap_t* map, dfk_strmap_it* it)
{
  /** @todo remove the line below after dfk_avltree_erase is implemented */
  return dfk_err_not_implemented;

  if (!map || !it) {
    return dfk_err_badarg;
  }
  dfk_avltree_erase(&map->_cont, it->_.value);
  return dfk_err_ok;
}


int dfk_strmap_insert(dfk_strmap_t* map, dfk_strmap_item_t* item)
{
  if (!map || !item) {
    return dfk_err_badarg;
  }
  dfk_avltree_insert(&map->_cont, &item->_hook);
  return dfk_err_ok;
}


int dfk_strmap_erase_find(dfk_strmap_t* map, const char* key, size_t keylen)
{
  /** @todo remove the line below after dfk_avltree_erase is implemented */
  return dfk_err_not_implemented;

  if (!map || (!key && keylen)) {
    return dfk_err_badarg;
  }
  dfk_strmap_it it;
  int ret = dfk_strmap_find(map, key, keylen, &it);
  if (ret != dfk_err_ok) {
    return ret;
  }
  return dfk_strmap_erase(map, &it);
}


int dfk_strmap_begin(dfk_strmap_t* map, dfk_strmap_it* it)
{
  if (!map || !it) {
    return dfk_err_badarg;
  }
  dfk_avltree_it_init(&map->_cont, &it->_);
  return dfk_err_ok;
}


int dfk_strmap_it_next(dfk_strmap_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  if (!dfk_avltree_it_valid(&it->_)) {
    return dfk_err_eof;
  }
  dfk_avltree_it_next(&it->_);
  return dfk_err_ok;
}


int dfk_strmap_it_valid(dfk_strmap_it* it)
{
  if (!it) {
    return dfk_err_badarg;
  }
  if (!dfk_avltree_it_valid(&it->_)) {
    return dfk_err_eof;
  }
  return dfk_err_ok;
}


int dfk_strmap_find(dfk_strmap_t* map, const char* key, size_t keylen,
                    dfk_strmap_it* it)
{
  if (!map || (!key && keylen) || !it) {
    return dfk_err_badarg;
  }
  dfk_buf_t lookup = {(char*) key, keylen};
  dfk_avltree_hook_t* i = dfk_avltree_lookup(&map->_cont, &lookup,
                                             dfk_strmap_lookup_cmp);
  if (!i) {
    return dfk_err_notfound;
  }
  it->_.value = i;
  return dfk_err_ok;
}


/** @} */

