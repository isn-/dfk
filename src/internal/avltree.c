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
#include <dfk/internal.h>
#include <dfk/internal/avltree.h>


#ifdef NDEBUG
#define DFK_AVLTREE_CHECK_INVARIANTS(tree) DFK_UNUSED(tree)
#else

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
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
  printf("%p (%hhd) [parent %p]\n", (void*) hook, hook->bal, (void*) hook->parent);
}


/* Allow recursion for debug builds */
static size_t dfk__avltree_check_invariants_node(dfk_avltree_hook_t* node)
{
  assert(node);
  ssize_t lheight = 0, rheight = 0;
  if (node->left) {
    assert(node->left->parent == node);
    lheight = dfk__avltree_check_invariants_node(node->left);
  }
  if (node->right) {
    assert(node->right->parent == node);
    rheight = dfk__avltree_check_invariants_node(node->right);
  }
  assert(abs((int) (lheight - rheight)) <= 1);
  assert(node->bal == (signed char) (rheight - lheight));
  return 1 + DFK_MAX(lheight, rheight);
}


static void dfk__avltree_check_invariants(dfk_avltree_t* tree)
{
  DFK_UNUSED(dfk__avltree_print);
  DFK_UNUSED(dfk__avltree_print_ptr);
  assert(tree);
  if (tree->root) {
    assert(!tree->root->parent);
    dfk__avltree_check_invariants_node(tree->root);
  }
  assert(tree->cmp);
}

#define DFK_AVLTREE_CHECK_INVARIANTS(tree) dfk__avltree_check_invariants((tree))
#endif /* NDEBUG */


typedef void (*dfk_avltree_traversal_cb)(dfk_avltree_t*, dfk_avltree_hook_t*);

static void dfk__avltree_dfs(dfk_avltree_t* tree, dfk_avltree_hook_t* hook, dfk_avltree_traversal_cb cb)
{
  if (!hook) {
    return;
  }
  dfk__avltree_dfs(tree, hook->left, cb);
  dfk__avltree_dfs(tree, hook->right, cb);
  cb(tree, hook);
}


void dfk_avltree_init(dfk_avltree_t* tree, dfk_avltree_cmp cmp)
{
  assert(tree);
  assert(cmp);
  tree->cmp = cmp;
  tree->root = NULL;
  tree->size = 0;
}


static void dfk__avltree_free_cb(dfk_avltree_t* tree, dfk_avltree_hook_t* hook)
{
  DFK_UNUSED(tree);
  dfk_avltree_hook_free(hook);
}


void dfk_avltree_free(dfk_avltree_t* tree)
{
  assert(tree);
  dfk__avltree_dfs(tree, tree->root, dfk__avltree_free_cb);
}


void dfk_avltree_hook_init(dfk_avltree_hook_t* hook)
{
  assert(hook);
  hook->left = NULL;
  hook->right = NULL;
  hook->parent = NULL;
  hook->bal = 0;
#if DFK_DEBUG
  hook->tree = NULL;
#endif
}


void dfk_avltree_hook_free(dfk_avltree_hook_t* hook)
{
  assert(hook);
  hook->left = NULL;
  hook->right = NULL;
  hook->parent = NULL;
  hook->bal = 0;
#if DFK_DEBUG
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
   */
  if (prime->bal == 2) {
    /*
     * Following rotation is performed:
     *
     *     ┌─c (height = h+1)              ┌─c
     *   ┌─B                             P─B
     *   │ └─b (height = h, or h+1)  =>    │ ┌─b
     * P─A                                 └─A
     *   └─a (height = h)                    └─a
     *
     */
    x = prime->right;
    assert(x);
    assert(x->bal == 0 || x->bal == 1);
    prime->right = x->left;
    if (x->left) {
      x->left->parent = prime;
    }
    x->left = prime;
    prime->parent = x;
    prime->bal = 1 - x->bal;
    x->bal = x->bal - 1;
  } else {
    /*
     * Following rotation is performed:
     *
     *   ┌─a (height = h)                  ┌─a
     * P─A                               ┌─A
     *   │ ┌─b (height = h, or h+1)  =>  │ └─b
     *   └─B                             P─B
     *     └─c (height = h+1)              └─c
     */
    x = prime->left;
    assert(prime->bal == -2);
    assert(x);
    assert(x->bal == 0 || x->bal == -1);
    prime->left = x->right;
    if (x->right) {
      x->right->parent = prime;
    }
    x->right = prime;
    prime->parent = x;
    prime->bal = -1 - x->bal;
    /*
     * height(b) == h, therefore x->bal == -1, then set x->bal = 0
     * height(b) == h + 1, therefore x->bal == 0, then set x->bal = 1
     */
    x->bal = x->bal + 1;
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
    if (x->left) {
      x->left->parent = prime;
    }
    x->left = prime;
    prime->parent = x;
    b->left = x->right;
    if (x->right) {
      x->right->parent = b;
    }
    x->right = b;
    b->parent = x;
    switch (x->bal) {
      case -1: {
        b->bal = 1;
        prime->bal = 0;
        break;
      }
      case 0: {
        b->bal = 0;
        prime->bal = 0;
        break;
      }
      case 1: {
        b->bal = 0;
        prime->bal = -1;
        break;
      }
    }
    x->bal = 0;
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
    if (x->right) {
      x->right->parent = prime;
    }
    x->right = prime;
    prime->parent = x;
    b->right = x->left;
    if (x->left) {
      x->left->parent = b;
    }
    x->left = b;
    b->parent = x;
    switch (x->bal) {
      case -1: {
        b->bal = 0;
        prime->bal = 1;
        break;
      }
      case 0: {
        b->bal = 0;
        prime->bal = 0;
        break;
      }
      case 1: {
        b->bal = -1;
        prime->bal = 0;
        break;
      }
    }
    x->bal = 0;
  }
  return x;
}


dfk_avltree_hook_t* dfk_avltree_insert(dfk_avltree_t* tree, dfk_avltree_hook_t* e)
{
  assert(tree);
  assert(e);
  assert(e->left == NULL);
  assert(e->right == NULL);
  assert(e->parent == NULL);
  assert(e->bal == 0);
  DFK_AVLTREE_CHECK_INVARIANTS(tree);

  if (tree->root == NULL) {
    tree->root = e;
    tree->size = 1;
    return tree->root;
  }

  {
    dfk_avltree_hook_t* i = tree->root;
    dfk_avltree_hook_t* prime_parent = NULL;
    dfk_avltree_hook_t* prime = tree->root;
    int prime_parent_dir = 0;

    while (1) {
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
          e->parent = i;
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
          e->parent = i;
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
      } else  {
        assert(cmp > 0);
        i->bal += 1;
        i = i->right;
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
      if (new_prime) {
        switch (prime_parent_dir) {
          case -1: {
             prime_parent->left = new_prime;
             new_prime->parent = prime_parent;
             break;
          }
          case 0: {
            tree->root = new_prime;
            new_prime->parent = NULL;
            break;
          }
          case 1: {
            prime_parent->right = new_prime;
            new_prime->parent = prime_parent;
            break;
          }
        }
      }
    }
  }
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  tree->size += 1;
  return e;
}


void dfk_avltree_erase(dfk_avltree_t* tree, dfk_avltree_hook_t* e)
{
  assert(tree);
  assert(e);
  assert(tree->size);
#if DFK_DEBUG
  assert(tree == e->tree);
#endif
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  /* Stores a pointer to node that should take e's place, may be NULL */
  dfk_avltree_hook_t* newe;
  /* Node to start rebalancing from */
  dfk_avltree_hook_t* prime;
  /* Is set to 1 if subtree's height has changed after the erase operation.
   * Height can only decrease, obviously.
   * If height has decreased, we need to update node's balance factor, and
   * possibly perform re-balancing, e.g. rotation to preserve AVL property.
   */
  int erase_direction = 0;
  if (!e->right) {
    /*
     * Simple case - e has no right subtree.
     * Replace e with it's left subtree.
     * Rebalance P moving upwards.
     *
     *  P─e    ->  P─A
     *    └─A
     */
    newe = e->left;
    prime = e->parent;
    erase_direction = prime ? (prime->left == e ? -1 : 1) : 0;
  } else if (!e->right->left) {
    /*
     * A little more tricky - e has right child R and R has no left child.
     * Replace e with it's right subtree and make e's left subtree new R's
     * left subtree.
     * Rebalance R moving upwards.
     *
     *      ┌─Q
     *    ┌─R          ┌─Q
     *  P─e      ->  P─R
     *    └─A          └─A
     */
    dfk_avltree_hook_t* r = e->right;
    dfk_avltree_hook_t* a = e->left;
    r->left = a;
    if (a) {
      a->parent = r;
    }
    r->bal = e->bal;
    /* Since e's right subtree replaces e on it's position,
     * height of R subtree after removal proceudre decreases
     * only if height(R) > heigth(A), or, in terms of nodes' balance, if e->bal == 1
     */
    newe = r;
    prime = r;
    erase_direction = 1;
  } else {
    /*
     * Most tricky case - e has right child R and R has a left child L.
     * Go down to the leftmost leaf in R. Let's call it Y. And it's parent - X.
     * X maybe be equal to L. Swap e and Y. Now e has no left subtree and could
     * be replace with it's right subtree which used to be Y's right subtree (Z).
     * Rebalance X moving upwards.
     *
     *      ┌─Q                ┌─Q
     *    ┌─R                ┌─R
     *    | └─L              | └─L
     *    |   :.X      ->    |   :.X
     *    |     | ┌─Z        |     └─Z
     *    |     └─Y        P─Y
     *  P─e                  └─A
     *    └─A
     */
    dfk_avltree_hook_t* r = e->right;
    dfk_avltree_hook_t* a = e->left;
    dfk_avltree_hook_t* i = r;
    /* Lookup for X node */
    assert(i->left);
    while (i->left->left) {
      i = i->left;
    }
    dfk_avltree_hook_t* x = i;
    dfk_avltree_hook_t* y = i->left;
    assert(y);
    assert(!y->left);
    dfk_avltree_hook_t* z = y->right;
    y->left = e->left;
    y->right = e->right;
    y->bal = e->bal;
    r->parent = y;
    a->parent = y;
    x->left = z;
    if (z) {
      z->parent = x;
    }
    newe = y;
    prime = x;
    erase_direction = -1;
  }

  if (e->parent) {
    if (e->parent->left == e) {
      e->parent->left = newe;
    } else {
      e->parent->right = newe;
    }
  } else {
    tree->root = newe;
  }
  if (newe) {
    newe->parent = e->parent;
  }

  while (prime) {
    if (erase_direction == -1) {
      /* A node was deleted from the left subtree */
      prime->bal++;
      /*
       * If prime->bal equals to 0, no rebalancing is required,
       * but we still need to proceed upwards in tree, because
       * height of the parent subtree might have been changed
       */
      if (prime->bal == 1) {
        /* Before deletion balance was 0, so left and right subtrees were of the same
         * height. After node deletion height of the left subtree decrased by 1, but
         * overall height of the prime remained the same = height of the right subtree
         * plus 1
         */
        break;
      }
      if (prime->bal == 2) {
        /*
         * AVL condition is violated - need rotate.
         * After the rotation, height of the tree will decrease by 1
         */
        dfk_avltree_hook_t* parent = prime->parent;
        int dir = parent ? (parent->left == prime ? -1 : 1) : 0;
        int rightbal = prime->right->bal;
        dfk_avltree_hook_t* new_prime;
        if (rightbal == -1) {
          new_prime = dfk__avltree_double_rot(prime);
        } else {
          new_prime = dfk__avltree_single_rot(prime);
        }
        switch (dir) {
          case -1:
            parent->left = new_prime;
            new_prime->parent = parent;
            break;
          case 0:
            tree->root = new_prime;
            tree->root->parent = NULL;
            break;
          case 1:
            parent->right = new_prime;
            new_prime->parent = parent;
            break;
        }
        prime = new_prime;
        if (!rightbal) {
          break;
        }
      }
    } else {
      assert(erase_direction == 1);
      prime->bal--;
      if (prime->bal == -1) {
        break;
      }
      if (prime->bal == -2) {
        dfk_avltree_hook_t* parent = prime->parent;
        int dir = parent ? (parent->left == prime ? -1 : 1) : 0;
        int leftbal = prime->left->bal;
        dfk_avltree_hook_t* new_prime;
        if (leftbal == 1) {
          new_prime = dfk__avltree_double_rot(prime);
        } else {
          new_prime = dfk__avltree_single_rot(prime);
        }
        switch (dir) {
          case -1:
            parent->left = new_prime;
            new_prime->parent = parent;
            break;
          case 0:
            tree->root = new_prime;
            tree->root->parent = NULL;
            break;
          case 1:
            parent->right = new_prime;
            new_prime->parent = parent;
            break;
        }
        prime = new_prime;
        if (!leftbal) {
          break;
        }
      }
    }
    erase_direction = prime->parent ? (prime->parent->left == prime ? -1 : 1) : 0;
    prime = prime->parent;
  }

  tree->size--;
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
#if DFK_DEBUG
  e->tree = NULL;
#endif
}


size_t dfk_avltree_size(dfk_avltree_t* tree)
{
  assert(tree);
  return tree->size;
}


dfk_avltree_hook_t* dfk_avltree_lookup(dfk_avltree_t* tree, void* e, dfk_avltree_lookup_cmp cmp)
{
  assert(tree);
  assert(e);
  assert(cmp);
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
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


void dfk_avltree_it_init(dfk_avltree_t* tree, dfk_avltree_it_t* it)
{
  assert(tree);
  assert(it);
  it->value = tree->root;
  if (it->value) {
    while (it->value->left) {
      it->value = it->value->left;
    }
  }
}


void dfk_avltree_it_free(dfk_avltree_it_t* it)
{
  assert(it);
  it->value = NULL;
}


void dfk_avltree_it_next(dfk_avltree_it_t* it)
{
  assert(it->value); /* can not increment end iterator */
  if (it->value->right) {
    it->value = it->value->right;
    while (it->value->left) {
      it->value = it->value->left;
    }
    return;
  }
  if (it->value->parent && it->value->parent->left == it->value) {
    it->value = it->value->parent;
    return;
  }
  while (it->value->parent && it->value->parent->right == it->value) {
    it->value = it->value->parent;
  }
  it->value = it->value->parent;
}


int dfk_avltree_it_valid(dfk_avltree_it_t* it)
{
  assert(it);
  return it->value != NULL;
}

