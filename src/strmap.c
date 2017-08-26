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
    dfk_buf_t key, dfk_buf_t value)
{
  assert(item);
  assert(key.data);
  assert(key.size);
  assert(value.data || !value.size);
  dfk_avltree_hook_init(&item->_hook);
  item->key = key;
  item->value = value;
}

dfk_strmap_item_t* dfk_strmap_item_copy(
    dfk_t* dfk, dfk_buf_t key, dfk_buf_t value)
{
  assert(dfk);
  assert(key.data);
  assert(key.size);
  assert(value.data || !value.size);
  /* Store key, value and item_t in the one memory block, save 2 alloc's */
  size_t toalloc = sizeof(dfk_strmap_item_t) + key.size + value.size;
  dfk_strmap_item_t* item = dfk__malloc(dfk, toalloc);
  if (!item) {
    return NULL;
  }
  char* pkey = (char*) item + sizeof(dfk_strmap_item_t);
  char* pvalue = pkey + key.size;
  memcpy(pkey, key.data, key.size);
  memcpy(pvalue, value.data, value.size);
  dfk_strmap_item_init(item,
      (dfk_buf_t) {pkey, key.size}, (dfk_buf_t) {pvalue, value.size});
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_copy_key(dfk_t* dfk,
    dfk_buf_t key, dfk_buf_t value)
{
  assert(dfk);
  assert(key.data);
  assert(key.size);
  assert(value.data || !value.size);
  /* Store key and item_t in the one memory block, save 1 alloc */
  size_t toalloc = sizeof(dfk_strmap_item_t) + key.size;
  dfk_strmap_item_t* item = dfk__malloc(dfk, toalloc);
  if (!item) {
    return NULL;
  }
  char* pkey = (char*) item + sizeof(dfk_strmap_item_t);
  memcpy(pkey, key.data, key.size);
  dfk_strmap_item_init(item, (dfk_buf_t) {pkey, key.size}, value);
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_copy_value(dfk_t* dfk,
    dfk_buf_t key, dfk_buf_t value)
{
  assert(dfk);
  assert(key.data);
  assert(key.size);
  assert(value.data || !value.size);
  /* Store value and item_t in the one memory block, save 1 alloc */
  size_t toalloc = sizeof(dfk_strmap_item_t) + value.size;
  dfk_strmap_item_t* item = dfk__malloc(dfk, toalloc);
  if (!item) {
    return NULL;
  }
  char* pvalue = (char*) item + sizeof(dfk_strmap_item_t);
  memcpy(pvalue, value.data, value.size);
  dfk_strmap_item_init(item, key, (dfk_buf_t) {pvalue, value.size});
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_acopy(dfk_arena_t* arena,
    dfk_buf_t key, dfk_buf_t value)
{
  assert(arena);
  assert(arena->dfk);
  assert(key.data);
  assert(key.size);
  assert(value.data || !value.size);
  char* pkey = dfk_arena_alloc_copy(arena, key.data, key.size);
  if (!pkey) {
    return NULL;
  }
  char* pvalue = dfk_arena_alloc_copy(arena, value.data, value.size);
  if (!pvalue) {
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return NULL;
  }
  dfk_strmap_item_init(item,
      (dfk_buf_t) {pkey, key.size}, (dfk_buf_t) {pvalue, value.size});
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_acopy_key(dfk_arena_t* arena,
    dfk_buf_t key, dfk_buf_t value)
{
  assert(arena);
  assert(arena->dfk);
  assert(key.data);
  assert(key.size);
  assert(value.data || !value.size);
  char* pkey = dfk_arena_alloc_copy(arena, key.data, key.size);
  if (!pkey) {
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return NULL;
  }
  dfk_strmap_item_init(item, (dfk_buf_t) {pkey, key.size}, value);
  return item;
}

dfk_strmap_item_t* dfk_strmap_item_acopy_value(dfk_arena_t* arena,
    dfk_buf_t key, dfk_buf_t value)
{
  assert(arena);
  assert(arena->dfk);
  assert(key.data);
  assert(key.size);
  assert(value.data || !value.size);
  char* pvalue = dfk_arena_alloc_copy(arena, value.data, value.size);
  if (!pvalue) {
    return NULL;
  }
  dfk_strmap_item_t* item = dfk_arena_alloc(arena, sizeof(dfk_strmap_item_t));
  if (!item) {
    return NULL;
  }
  dfk_strmap_item_init(item, key, (dfk_buf_t) {pvalue, value.size});
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

dfk_buf_t dfk_strmap_get(dfk_strmap_t* map, dfk_buf_t key)
{
  assert(map);
  assert(key.data);
  assert(key.size);
  dfk_strmap_item_t* item = TO_STRMAP_ITEM(
      dfk_avltree_find(&map->_cont, &key, dfk__strmap_lookup_cmp));
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

