/**
 * @file dfk/strmap.h
 * Key-value container with keys and values of type dfk_buf_t
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <dfk/core.h>
#include <dfk/internal/arena.h>
#include <dfk/internal/avltree.h>

/**
 * @addtogroup util util
 * @{
 */

/**
 * Element of dfk_strmap_t
 */
typedef struct dfk_strmap_item_t {
  /** @private */
  dfk_avltree_hook_t _hook;
  dfk_buf_t key;
  dfk_buf_t value;
} dfk_strmap_item_t;


/**
 * Associative container, maps dfk_buf_t to dfk_buf_t
 */
typedef struct dfk_strmap_t {
  /** @private */
  dfk_avltree_t _cont;
} dfk_strmap_t;


/**
 * dfk_strmap_t iterator
 */
typedef struct dfk_strmap_it {
  union {
    dfk_avltree_it_t _;
    dfk_strmap_item_t* item;
  };
} dfk_strmap_it;


int dfk_strmap_item_init(dfk_strmap_item_t* item,
                         const char* key, size_t keylen,
                         const char* value, size_t valuelen);

dfk_strmap_item_t* dfk_strmap_item_copy(
    dfk_t* dfk,
    const char* key, size_t keylen,
    const char* value, size_t valuelen);

dfk_strmap_item_t* dfk_strmap_item_copy_key(
    dfk_t* dfk,
    const char* key, size_t keylen,
    const char* value, size_t valuelen);

dfk_strmap_item_t* dfk_strmap_item_copy_value(
    dfk_t* dfk,
    const char* key, size_t keylen,
    const char* value, size_t valuelen);

dfk_strmap_item_t* dfk_strmap_item_acopy(
    dfk_arena_t* arena,
    const char* key, size_t keylen,
    const char* value, size_t valuelen);

dfk_strmap_item_t* dfk_strmap_item_acopy_key(
    dfk_arena_t* arena,
    const char* key, size_t keylen,
    const char* value, size_t valuelen);

dfk_strmap_item_t* dfk_strmap_item_acopy_value(
    dfk_arena_t* arena,
    const char* key, size_t keylen,
    const char* value, size_t valuelen);

int dfk_strmap_item_free(dfk_strmap_item_t* item);

int dfk_strmap_init(dfk_strmap_t* map);
int dfk_strmap_free(dfk_strmap_t* map);

/**
 * Returns size of the dfk_strmap_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_strmap_sizeof(void);
size_t dfk_strmap_size(dfk_strmap_t* map);
dfk_buf_t dfk_strmap_get(dfk_strmap_t* map, const char* key, size_t keylen);
int dfk_strmap_insert(dfk_strmap_t* map, dfk_strmap_item_t* item);
int dfk_strmap_erase(dfk_strmap_t* map, dfk_strmap_it* it);
int dfk_strmap_erase_find(dfk_strmap_t* map, const char* key, size_t keylen);

int dfk_strmap_begin(dfk_strmap_t* map, dfk_strmap_it* it);
int dfk_strmap_it_next(dfk_strmap_it* it);
int dfk_strmap_it_valid(dfk_strmap_it* it);

int dfk_strmap_find(dfk_strmap_t* map, const char* key, size_t keylen,
                    dfk_strmap_it* it);

/** @} */

