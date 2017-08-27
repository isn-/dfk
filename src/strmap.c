/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <string.h>
#include <dfk/strmap.h>
#include <dfk/internal.h>
#include <dfk/malloc.h>

#define TO_STRMAP_ITEM(expr) DFK_CONTAINER_OF((expr), dfk_strmap_item_t, _hook)

static int dfk__strmap_lookup_cmp(dfk_avltree_hook_t* l, void* r)
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

static int dfk__strmap_cmp(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r)
{
  assert(l);
  assert(r);
  return dfk__strmap_lookup_cmp(l, &((dfk_strmap_item_t*) r)->key);
}

void dfk_strmap_item_init(dfk_strmap_item_t* item,
    const char* key, size_t keylen, char* value, size_t valuelen)
{
  assert(item);
  assert(key);
  assert(keylen);
  assert(value || !valuelen);
  dfk_avltree_hook_init(&item->_hook);
  item->key = (dfk_cbuf_t) {key, keylen};
  item->value = (dfk_buf_t) {value, valuelen};
}

dfk_strmap_item_t* dfk_strmap_item_copy(dfk_t* dfk,
    const char* key, size_t keylen, const char* value, size_t valuelen)
{
  assert(dfk);
  assert(key);
  assert(keylen);
  assert(value || !valuelen);
  /* Store key, value and item_t in the one memory block, save 2 alloc's */
  size_t toalloc = sizeof(dfk_strmap_item_t) + keylen + valuelen;
  dfk_strmap_item_t* item = dfk__malloc(dfk, toalloc);
  if (!item) {
    return NULL;
  }
  char* pkey = (char*) item + sizeof(dfk_strmap_item_t);
  char* pvalue = pkey + keylen;
  memcpy(pkey, key, keylen);
  memcpy(pvalue, value, valuelen);
  dfk_strmap_item_init(item, pkey, keylen, pvalue, valuelen);
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_copy_key(dfk_t* dfk,
    const char* key, size_t keylen, char* value, size_t valuelen)
{
  assert(dfk);
  assert(key);
  assert(keylen);
  assert(value || !valuelen);
  /* Store key and item_t in the one memory block, save 1 alloc */
  size_t toalloc = sizeof(dfk_strmap_item_t) + keylen;
  dfk_strmap_item_t* item = dfk__malloc(dfk, toalloc);
  if (!item) {
    return NULL;
  }
  char* pkey = (char*) item + sizeof(dfk_strmap_item_t);
  memcpy(pkey, key, keylen);
  dfk_strmap_item_init(item, pkey, keylen, value, valuelen);
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_copy_value(dfk_t* dfk,
    const char* key, size_t keylen, const char* value, size_t valuelen)
{
  assert(dfk);
  assert(key);
  assert(keylen);
  assert(value || !valuelen);
  /* Store value and item_t in the one memory block, save 1 alloc */
  size_t toalloc = sizeof(dfk_strmap_item_t) + valuelen;
  dfk_strmap_item_t* item = dfk__malloc(dfk, toalloc);
  if (!item) {
    return NULL;
  }
  char* pvalue = (char*) item + sizeof(dfk_strmap_item_t);
  memcpy(pvalue, value, valuelen);
  dfk_strmap_item_init(item, key, keylen, pvalue, valuelen);
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_acopy(dfk_arena_t* arena,
    const char* key, size_t keylen, const char* value, size_t valuelen)
{
  assert(arena);
  assert(arena->dfk);
  assert(key);
  assert(keylen);
  assert(value || !valuelen);
  char* pkey = dfk_arena_alloc_copy(arena, key, keylen);
  if (!pkey) {
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
  dfk_strmap_item_init(item, pkey, keylen, pvalue, valuelen);
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_acopy_key(dfk_arena_t* arena,
    const char* key, size_t keylen, char* value, size_t valuelen)
{
  assert(arena);
  assert(arena->dfk);
  assert(key);
  assert(keylen);
  assert(value || !valuelen);
  char* pkey = dfk_arena_alloc_copy(arena, key, keylen);
  if (!pkey) {
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return NULL;
  }
  dfk_strmap_item_init(item, pkey, keylen, value, valuelen);
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_acopy_value(dfk_arena_t* arena,
    const char* key, size_t keylen, const char* value, size_t valuelen)
{
  assert(arena);
  assert(arena->dfk);
  assert(key);
  assert(keylen);
  assert(value || !valuelen);
  char* pvalue = dfk_arena_alloc_copy(arena, value, valuelen);
  if (!pvalue) {
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return NULL;
  }
  dfk_strmap_item_init(item, key, keylen, pvalue, valuelen);
  return item;
}

void dfk_strmap_init(dfk_strmap_t* map)
{
  assert(map);
  dfk_avltree_init(&map->_cont, dfk__strmap_cmp);
}

size_t dfk_strmap_sizeof(void)
{
  return sizeof(dfk_strmap_t);
}

size_t dfk_strmap_size(dfk_strmap_t* map)
{
  assert(map);
  return dfk_avltree_size(&map->_cont);
}

dfk_buf_t dfk_strmap_get(dfk_strmap_t* map, const char* key, size_t keylen)
{
  assert(map);
  assert(key);
  assert(keylen);
  dfk_cbuf_t kkey = (dfk_cbuf_t) {key, keylen};
  dfk_strmap_item_t* item = TO_STRMAP_ITEM(
      dfk_avltree_find(&map->_cont, &kkey, dfk__strmap_lookup_cmp));
  return item ? item->value : (dfk_buf_t) {NULL, 0};
}

void dfk_strmap_insert(dfk_strmap_t* map, dfk_strmap_item_t* item)
{
  assert(map);
  assert(item);
  dfk_avltree_insert(&map->_cont, &item->_hook);
}

void dfk_strmap_erase(dfk_strmap_t* map, dfk_strmap_it* it)
{
  assert(map);
  assert(it);
  dfk_avltree_erase(&map->_cont, it->_it.value);
}

void dfk_strmap_begin(dfk_strmap_t* map, dfk_strmap_it* it)
{
  assert(map);
  assert(it);
  dfk_avltree_begin(&map->_cont, &it->_it);
}

void dfk_strmap_end(dfk_strmap_t* map, dfk_strmap_it* it)
{
  assert(map);
  assert(it);
  dfk_avltree_end(&map->_cont, &it->_it);
}

size_t dfk_strmap_it_sizeof(void)
{
  return sizeof(dfk_strmap_it);
}

void dfk_strmap_it_next(dfk_strmap_it* it)
{
  assert(it);
  dfk_avltree_it_next(&it->_it);
}

int dfk_strmap_it_equal(dfk_strmap_it* lhs, dfk_strmap_it* rhs)
{
  assert(lhs);
  assert(rhs);
  return dfk_avltree_it_equal(&lhs->_it, &rhs->_it);
}

