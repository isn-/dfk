/**
 * @file dfk/internal/avltree.h
 * Balanced tree data structure
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
#include <dfk/config.h>

typedef struct dfk_avltree_hook_t {
  struct dfk_avltree_hook_t* left;
  struct dfk_avltree_hook_t* right;
  struct dfk_avltree_hook_t* parent;
  signed char bal;
#if DFK_DEBUG
  struct dfk_avltree_t* tree;
#endif
} dfk_avltree_hook_t;

typedef int (*dfk_avltree_cmp)(dfk_avltree_hook_t*, dfk_avltree_hook_t*);
typedef int (*dfk_avltree_lookup_cmp)(dfk_avltree_hook_t*, void*);

typedef struct dfk_avltree_t {
  dfk_avltree_hook_t* root;
  dfk_avltree_cmp cmp;
  size_t size;
} dfk_avltree_t;


typedef struct dfk_avltree_it_t {
  dfk_avltree_hook_t* value;
} dfk_avltree_it_t;

void dfk_avltree_init(dfk_avltree_t* tree, dfk_avltree_cmp cmp);
void dfk_avltree_free(dfk_avltree_t* tree);

void dfk_avltree_hook_init(dfk_avltree_hook_t* h);
void dfk_avltree_hook_free(dfk_avltree_hook_t* h);

dfk_avltree_hook_t* dfk_avltree_insert(dfk_avltree_t* tree, dfk_avltree_hook_t* e);
void dfk_avltree_erase(dfk_avltree_t* tree, dfk_avltree_hook_t* e);
/** @todo rename to dfk_avltree_find */
dfk_avltree_hook_t* dfk_avltree_lookup(dfk_avltree_t* tree, void* e, dfk_avltree_lookup_cmp cmp);
size_t dfk_avltree_size(dfk_avltree_t* tree);

/** @todo rename to dfk_avltree_begin */
void dfk_avltree_it_init(dfk_avltree_t* tree, dfk_avltree_it_t* it);
void dfk_avltree_it_free(dfk_avltree_it_t* it);
void dfk_avltree_it_next(dfk_avltree_it_t* it);
int dfk_avltree_it_valid(dfk_avltree_it_t* it);

