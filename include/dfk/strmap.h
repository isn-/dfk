/**
 * @file dfk/strmap.h
 * Key-value container with keys and values of type dfk_buf_t
 *
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 *
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
#include <dfk/internal/arena.h>
#include <dfk/internal/avltree.h>

/**
 * @addtogroup util
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

