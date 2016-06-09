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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <dfk/internal.h>
#include <dfk/internal/avltree.h>


#ifdef NDEBUG
#define DFK_AVLTREE_CHECK_INVARIANTS(tree) DFK_UNUSED(tree)
#else

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef void (*dfk__avltree_print_cb)(dfk_avltree_hook_t* hook);
static void dfk__avltree_print(dfk_avltree_hook_t* tree, dfk__avltree_print_cb cb, size_t lvl, int64_t mask, int orient)
{
  if (!tree) {
    return;
  }
  DFK_UNUSED(tree);
  {
    int64_t left_mask = mask | (1 << lvl);
    int64_t right_mask = mask | (1 << lvl);
    if (lvl) {
      if (orient == -1) {
        left_mask ^= (1 << (lvl - 1));
      } else if (orient == 1) {
        right_mask ^= (1 << (lvl - 1));
      }
    }
    dfk__avltree_print(tree->right, cb, lvl + 1, left_mask, -1);
    if (lvl) {
      size_t i;
      for (i = 0; i < lvl - 1; ++i) {
        if (mask & (1 << i)) {
          printf("│ ");
        } else {
          printf("  ");
        }
      }
      if (orient == -1) {
        printf("┌─");
      } else if (orient == 1) {
        printf("└─");
      }
    }
    cb(tree);
    dfk__avltree_print(tree->left, cb, lvl + 1, right_mask, 1);
  }
}


static void dfk__avltree_print_ptr(dfk_avltree_hook_t* hook)
{
  printf("%p (%hhd)\n", (void*) hook, hook->bal);
}


static void dfk__avltree_check_invariants(dfk_avltree_t* tree)
{
  DFK_UNUSED(dfk__avltree_print);
  DFK_UNUSED(dfk__avltree_print_ptr);
  DFK_UNUSED(tree);
}

#define DFK_AVLTREE_CHECK_INVARIANTS(tree) dfk__avltree_check_invariants((tree))
#endif /* NDEBUG */


static int dfk__avltree_dfs(dfk_avltree_t* tree, dfk_avltree_hook_t* hook, dfk_avltree_traversal_cb cb)
{
  int err;
  if (!hook) {
    return 0;
  }
  if ((err = dfk__avltree_dfs(tree, hook->left, cb)) != 0) {
    return err;
  }
  if ((err = dfk__avltree_dfs(tree, hook->right, cb)) != 0) {
    return err;
  }
  return cb(tree, hook);
}


void dfk_avltree_init(dfk_avltree_t* tree, dfk_avltree_cmp cmp)
{
  assert(tree);
  assert(cmp);
  tree->cmp = cmp;
  tree->root = NULL;
}


static int dfk__avltree_free_cb(dfk_avltree_t* tree, dfk_avltree_hook_t* hook)
{
  DFK_UNUSED(tree);
  dfk_avltree_hook_free(hook);
  return 0;
}


void dfk_avltree_free(dfk_avltree_t* tree)
{
  assert(tree);
  (void) dfk__avltree_dfs(tree, tree->root, dfk__avltree_free_cb);
}


void dfk_avltree_hook_init(dfk_avltree_hook_t* hook)
{
  assert(hook);
  hook->left = NULL;
  hook->right = NULL;
  hook->bal = 0;
#ifdef DFK_DEBUG
  hook->tree = NULL;
#endif
}


void dfk_avltree_hook_free(dfk_avltree_hook_t* hook)
{
  assert(hook);
  hook->left = NULL;
  hook->right = NULL;
  hook->bal = 0;
#ifdef DFK_DEBUG
  hook->tree = NULL;
#endif
}


static dfk_avltree_hook_t* dfk__avltree_single_rot(dfk_avltree_hook_t* prime)
{
  dfk_avltree_hook_t* x;
  /*
   * Input arguments are
   * prime: (A)
   * parent node relatively to the prime: (P)
   * "x" variable stores a pointer to the "B" node
   *
   */
  if (prime->bal == 2) {
    /*
     * Following rotation is performed:
     *
     *     ┌─c (height = h+1)        ┌─c
     *   ┌─B                       P─B
     *   │ └─b (height = h)    =>    │ ┌─b
     * P─A                           └─A
     *   └─a (height = h)              └─a
     *
     */
    x = prime->right;
    assert(x);
    assert(x->bal == 1);
    prime->right = x->left;
    x->left = prime;
    prime->bal = 0;
    x->bal = 0;
  } else {
    /*
     * Following rotation is performed:
     *
     *   ┌─a (height = h)              ┌─a
     * P─A                           ┌─A
     *   │ ┌─b (height = h)    =>    │ └─b
     *   └─B                       P─B
     *     └─c (height = h+1)        └─c
     */
    x = prime->left;
    assert(prime->bal == -2);
    assert(x);
    assert(x->bal == -1);
    prime->left = x->right;
    x->right = prime;
    prime->bal = 0;
    x->bal = 0;
  }
  return x;
}


static dfk_avltree_hook_t* dfk__avltree_double_rot(dfk_avltree_hook_t* prime)
{
  dfk_avltree_hook_t* x;
  /*
   * Input arguments are
   * prime: (A)
   * parent node relatively to the prime: (P)
   * Local variables refer to their capital-letter equivalents
   * (b to B node, x to X node)
   */
  if (prime->bal == 2) {
    /*
     * Following rotation is performed:
     *
     *     ┌─d (height = h)              ┌─d
     *   ┌─B                           ┌─B
     *   │ │ ┌─c (height = h)          │ └─c
     *   │ └─X                   =>  P─X
     *   │   └─b (height = h-1)        │ ┌─b
     * P─A                             └─A
     *   └─a (height = h)                └─a
     */
    dfk_avltree_hook_t* b = prime->right;
    x = prime->right->left;
    prime->right = x->left;
    x->left = prime;
    b->left = x->right;
    x->right = b;
    b->bal = 0;
    x->bal = 0;
    prime->bal = -1;
  } else {
    /*
     * Following rotation is performed:
     *
     *   ┌─a (height = h)                ┌─a
     * P─A                             ┌─A
     *   │   ┌─b (height = h-1)        │ └─b
     *   │ ┌─X                   =>  P─X
     *   │ │ └─c (height = h)          │ ┌─c
     *   └─B                           └─B
     *     └─d (height = h)              └─d
     */
    dfk_avltree_hook_t* b = prime->left;
    x = prime->left->right;
    prime->left = x->right;
    x->right = prime;
    b->right = x->left;
    x->left = b;
    b->bal = 0;
    x->bal = 0;
    prime->bal = 1;
  }
  return x;
}


dfk_avltree_hook_t* dfk_avltree_insert(dfk_avltree_t* tree, dfk_avltree_hook_t* e)
{
  assert(tree);
  assert(e);
  {
    dfk_avltree_hook_t* i = tree->root;
    dfk_avltree_hook_t* prime_parent = NULL;
    dfk_avltree_hook_t* prime = tree->root;
    int prime_parent_dir = 0;

    while (i) {
      int cmp = tree->cmp(i, e);
      if (cmp < 0) {
        if (i->left) {
          if (i->left->bal) {
            prime = i->left;
            prime_parent = i;
            prime_parent_dir = -1;
          }
          i = i->left;
        } else {
          i->left = e;
          break;
        }
      } else if (cmp > 0) {
        if (i->right) {
          if (i->right->bal) {
            prime = i->right;
            prime_parent = i;
            prime_parent_dir = 1;
          }
          i = i->right;
        } else {
          i->right = e;
          break;
        }
      } else {
        return NULL;
      }
    }

    i = prime;
    while (i != e) {
      int cmp = tree->cmp(i, e);
      if (cmp < 0) {
        i->bal -= 1;
        i = i->left;
      } else if (cmp > 0) {
        i->bal += 1;
        i = i->right;
      } else {
        assert(i == e);
        break;
      }
    }

    {
      dfk_avltree_hook_t* new_prime = NULL;
      if ((prime->bal == 2 && prime->right->bal == -1)
          || (prime->bal == -2 && prime->left->bal == 1)) {
        new_prime = dfk__avltree_double_rot(prime);
      } else if ((prime->bal == 2 && prime->right->bal == 1)
          || (prime->bal == -2 && prime->left->bal == -1)) {
        new_prime = dfk__avltree_single_rot(prime);
      }
      switch (prime_parent_dir) {
        case -1: {
           prime_parent->left = new_prime;
           break;
        }
        case 0: {
          tree->root = new_prime;
          break;
        }
        case 1: {
          prime_parent->right = new_prime;
          break;
        }
      }
    }
  }
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  return e;
}


void dfk_avltree_erase(dfk_avltree_t* tree, dfk_avltree_hook_t* e)
{
  DFK_UNUSED(tree);
  DFK_UNUSED(e);
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
}


dfk_avltree_hook_t* dfk_avltree_lookup(dfk_avltree_t* tree, void* e, dfk_avltree_lookup_cmp cmp)
{
  assert(tree);
  assert(e);
  assert(cmp);
  {
    dfk_avltree_hook_t* i = tree->root;
    while (i) {
      int cmpres = cmp(i, e);
      if (cmpres < 0) {
        i = i->left;
      } else if (cmpres > 0) {
        i = i->right;
      } else {
        return i;
      }
    }
  }
  return NULL;
}

