/**
 * @file dfk/avltree.h
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
  /**
   * @privatesection
   */
  struct dfk_avltree_hook_t* _left;
  struct dfk_avltree_hook_t* _right;
  struct dfk_avltree_hook_t* _parent;
  signed char _bal;
#if DFK_DEBUG
  struct dfk_avltree_t* _tree;
#endif
} dfk_avltree_hook_t;

typedef int (*dfk_avltree_cmp)(dfk_avltree_hook_t*, dfk_avltree_hook_t*);
typedef int (*dfk_avltree_find_cmp)(dfk_avltree_hook_t*, void*);

/**
 * AVL tree, self-balancing binary search tree
 *
 * @see https://en.wikipedia.org/wiki/AVL_tree
 */
typedef struct dfk_avltree_t {
  /**
   * @privatesection
   */
  dfk_avltree_hook_t* _root;
  dfk_avltree_cmp _cmp;
#if DFK_AVLTREE_CONSTANT_TIME_SIZE
  size_t _size;
#endif
} dfk_avltree_t;


/**
 * dfk_avltree_t iterator
 */
typedef struct dfk_avltree_it {
  dfk_avltree_hook_t* value;
#if DFK_DEBUG
  /** @private */
  dfk_avltree_t* _tree;
#endif
} dfk_avltree_it;

void dfk_avltree_init(dfk_avltree_t* tree, dfk_avltree_cmp cmp);

void dfk_avltree_hook_init(dfk_avltree_hook_t* h);

dfk_avltree_hook_t* dfk_avltree_insert(dfk_avltree_t* tree,
    dfk_avltree_hook_t* e);

void dfk_avltree_erase(dfk_avltree_t* tree, dfk_avltree_hook_t* e);

dfk_avltree_hook_t* dfk_avltree_find(dfk_avltree_t* tree, void* e,
    dfk_avltree_find_cmp cmp);

size_t dfk_avltree_size(dfk_avltree_t* tree);

size_t dfk_avltree_sizeof(void);

/**
 * @todo Add an option to switch between constant-time and
 * logarithmic dfk_avltree_begin.
 */
void dfk_avltree_begin(dfk_avltree_t* tree, dfk_avltree_it* it);

void dfk_avltree_end(dfk_avltree_t* tree, dfk_avltree_it* it);

void dfk_avltree_it_next(dfk_avltree_it* it);

int dfk_avltree_it_equal(dfk_avltree_it* lhs, dfk_avltree_it* rhs);

size_t dfk_avltree_it_sizeof(void);

