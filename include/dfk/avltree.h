/**
 * @file dfk/internal/avltree.h
 * Balanced tree data structure
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
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
typedef int (*dfk_avltree_find_cmp)(dfk_avltree_hook_t*, void*);

typedef struct dfk_avltree_t {
  dfk_avltree_hook_t* root;
  dfk_avltree_cmp cmp;
  size_t size;
} dfk_avltree_t;


typedef struct dfk_avltree_it {
  dfk_avltree_hook_t* value;
} dfk_avltree_it;

void dfk_avltree_init(dfk_avltree_t* tree, dfk_avltree_cmp cmp);
void dfk_avltree_free(dfk_avltree_t* tree);

void dfk_avltree_hook_init(dfk_avltree_hook_t* h);
void dfk_avltree_hook_free(dfk_avltree_hook_t* h);

dfk_avltree_hook_t* dfk_avltree_insert(dfk_avltree_t* tree,
    dfk_avltree_hook_t* e);
void dfk_avltree_erase(dfk_avltree_t* tree, dfk_avltree_hook_t* e);
dfk_avltree_hook_t* dfk_avltree_find(dfk_avltree_t* tree, void* e,
    dfk_avltree_find_cmp cmp);
size_t dfk_avltree_size(dfk_avltree_t* tree);
size_t dfk_avltree_sizeof(void);

void dfk_avltree_it_begin(dfk_avltree_t* tree, dfk_avltree_it* it);
void dfk_avltree_it_free(dfk_avltree_it* it);
void dfk_avltree_it_next(dfk_avltree_it* it);
int dfk_avltree_it_valid(dfk_avltree_it* it);
size_t dfk_avltree_it_sizeof(void);

