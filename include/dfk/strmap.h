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
#include <dfk/misc.h>
#include <dfk/context.h>
#include <dfk/arena.h>
#include <dfk/avltree.h>

/**
 * Element of dfk_strmap_t
 */
typedef struct dfk_strmap_item_t {
  /**
   * @private
   * @warning Should be the first struct member for dfk_strmap_it layout trick.
   */
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
    dfk_strmap_item_t* item;
    /** @private */
    dfk_avltree_it _it;
  };
} dfk_strmap_it;

void dfk_strmap_item_init(dfk_strmap_item_t* item,
    dfk_buf_t key, dfk_buf_t value);

dfk_strmap_item_t* dfk_strmap_item_copy(dfk_t* dfk,
    dfk_buf_t key, dfk_buf_t value);

dfk_strmap_item_t* dfk_strmap_item_copy_key(dfk_t* dfk,
    dfk_buf_t key, dfk_buf_t value);

dfk_strmap_item_t* dfk_strmap_item_copy_value(dfk_t* dfk,
    dfk_buf_t key, dfk_buf_t value);

dfk_strmap_item_t* dfk_strmap_item_acopy(dfk_arena_t* arena,
    dfk_buf_t key, dfk_buf_t value);

dfk_strmap_item_t* dfk_strmap_item_acopy_key(dfk_arena_t* arena,
    dfk_buf_t key, dfk_buf_t value);

dfk_strmap_item_t* dfk_strmap_item_acopy_value(dfk_arena_t* arena,
    dfk_buf_t key, dfk_buf_t value);

void dfk_strmap_init(dfk_strmap_t* map);

/**
 * Returns size of the dfk_strmap_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_strmap_sizeof(void);
size_t dfk_strmap_size(dfk_strmap_t* map);

dfk_buf_t dfk_strmap_get(dfk_strmap_t* map, dfk_buf_t key);

void dfk_strmap_insert(dfk_strmap_t* map, dfk_strmap_item_t* item);

void dfk_strmap_erase(dfk_strmap_t* map, dfk_strmap_it* it);

void dfk_strmap_begin(dfk_strmap_t* map, dfk_strmap_it* it);
void dfk_strmap_end(dfk_strmap_t* map, dfk_strmap_it* it);
void dfk_strmap_it_next(dfk_strmap_it* it);
int dfk_strmap_it_equal(dfk_strmap_it* lhs, dfk_strmap_it* rhs);
size_t dfk_strmap_it_sizeof(void);

