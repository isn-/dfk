/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <dfk/internal.h>
#include <dfk/avltree.h>

#if DFK_AVLTREE_CONSTANT_TIME_SIZE
#define DFK_IF_AVLTREE_CONSTANT_TIME_SIZE(expr) (expr)
#else
#define DFK_IF_AVLTREE_CONSTANT_TIME_SIZE(expr)
#endif


#ifdef NDEBUG

#define DFK_AVLTREE_CHECK_INVARIANTS(tree) DFK_UNUSED(tree)
#define DFK_AVLTREE_HOOK_CHECK_INVARIANTS(hook) DFK_UNUSED(hook)
#define DFK_AVLTREE_IT_CHECK_INVARIANTS(it) DFK_UNUSED(it)

#else

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

typedef void (*dfk__avltree_print_cb)(dfk_avltree_hook_t* hook);

static void dfk__avltree_print(dfk_avltree_hook_t* tree,
    dfk__avltree_print_cb cb, size_t lvl, int64_t mask, int orient)
{
  if (!tree) {
    return;
  }
  int64_t left_mask = mask | (1 << lvl);
  int64_t right_mask = mask | (1 << lvl);
  if (lvl) {
    if (orient == -1) {
      left_mask ^= (1 << (lvl - 1));
    } else if (orient == 1) {
      right_mask ^= (1 << (lvl - 1));
    }
  }
  dfk__avltree_print(tree->_right, cb, lvl + 1, left_mask, -1);
  if (lvl) {
    for (size_t i = 0; i < lvl - 1; ++i) {
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
  dfk__avltree_print(tree->_left, cb, lvl + 1, right_mask, 1);
}


static void dfk__avltree_print_ptr(dfk_avltree_hook_t* hook)
{
  printf("%p (%hhd) [parent %p]\n", (void*) hook, hook->_bal, (void*) hook->_parent);
}


/* Allow recursion for debug builds */
static size_t dfk__avltree_check_invariants_node(dfk_avltree_hook_t* node)
{
  assert(node);
  size_t lheight = 0, rheight = 0;
  if (node->_left) {
    assert(node->_left->_parent == node);
    lheight = dfk__avltree_check_invariants_node(node->_left);
  }
  if (node->_right) {
    assert(node->_right->_parent == node);
    rheight = dfk__avltree_check_invariants_node(node->_right);
  }
  assert(abs((int) (((long long) lheight) - rheight)) <= 1);
  assert(node->_bal == (signed char) (rheight - lheight));
  return 1 + DFK_MAX(lheight, rheight);
}


static void dfk__avltree_check_invariants(dfk_avltree_t* tree)
{
  DFK_UNUSED(dfk__avltree_print);
  DFK_UNUSED(dfk__avltree_print_ptr);
  assert(tree);
  if (tree->_root) {
    assert(!tree->_root->_parent);
    dfk__avltree_check_invariants_node(tree->_root);
  }
  assert(tree->_cmp);
}


static void dfk__avltree_hook_check_invariants(dfk_avltree_hook_t* hook)
{
  assert(hook);
}


static void dfk__avltree_it_check_invariants(dfk_avltree_it* it)
{
  assert(it);
}

#define DFK_AVLTREE_CHECK_INVARIANTS(tree) \
  dfk__avltree_check_invariants((tree))

#define DFK_AVLTREE_HOOK_CHECK_INVARIANTS(hook) \
  dfk__avltree_hook_check_invariants((hook))

#define DFK_AVLTREE_IT_CHECK_INVARIANTS(it) \
  dfk__avltree_it_check_invariants((it))

#endif /* NDEBUG */


void dfk_avltree_init(dfk_avltree_t* tree, dfk_avltree_cmp cmp)
{
  assert(tree);
  assert(cmp);
  tree->_cmp = cmp;
  tree->_root = NULL;
  DFK_IF_AVLTREE_CONSTANT_TIME_SIZE(tree->_size = 0);
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
}


void dfk_avltree_hook_init(dfk_avltree_hook_t* hook)
{
  assert(hook);
  hook->_left = NULL;
  hook->_right = NULL;
  hook->_parent = NULL;
  hook->_bal = 0;
  DFK_IF_DEBUG(hook->_tree = NULL);
  DFK_AVLTREE_HOOK_CHECK_INVARIANTS(hook);
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
  if (prime->_bal == 2) {
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
    x = prime->_right;
    assert(x);
    assert(x->_bal == 0 || x->_bal == 1);
    prime->_right = x->_left;
    if (x->_left) {
      x->_left->_parent = prime;
    }
    x->_left = prime;
    prime->_parent = x;
    prime->_bal = 1 - x->_bal;
    x->_bal = x->_bal - 1;
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
    x = prime->_left;
    assert(prime->_bal == -2);
    assert(x);
    assert(x->_bal == 0 || x->_bal == -1);
    prime->_left = x->_right;
    if (x->_right) {
      x->_right->_parent = prime;
    }
    x->_right = prime;
    prime->_parent = x;
    prime->_bal = -1 - x->_bal;
    /*
     * height(b) == h, therefore x->_bal == -1, then set x->_bal = 0
     * height(b) == h + 1, therefore x->_bal == 0, then set x->_bal = 1
     */
    x->_bal = x->_bal + 1;
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
  if (prime->_bal == 2) {
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
    dfk_avltree_hook_t* b = prime->_right;
    x = prime->_right->_left;
    prime->_right = x->_left;
    if (x->_left) {
      x->_left->_parent = prime;
    }
    x->_left = prime;
    prime->_parent = x;
    b->_left = x->_right;
    if (x->_right) {
      x->_right->_parent = b;
    }
    x->_right = b;
    b->_parent = x;
    /*
     * x->_bal | b->_bal
     * --------+--------
     *   -1    |   1
     *    0    |   0
     *   +1    |   0
     */
    b->_bal = !(x->_bal + 1);
    /*
     * x->_bal | prime->_bal
     * --------+--------
     *   -1    |   0
     *    0    |   0
     *   +1    |  -1
     */
    prime->_bal = 0 - !(x->_bal - 1);
    x->_bal = 0;
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
    dfk_avltree_hook_t* b = prime->_left;
    x = prime->_left->_right;
    prime->_left = x->_right;
    if (x->_right) {
      x->_right->_parent = prime;
    }
    x->_right = prime;
    prime->_parent = x;
    b->_right = x->_left;
    if (x->_left) {
      x->_left->_parent = b;
    }
    x->_left = b;
    b->_parent = x;
    /*
     * x->_bal | b->_bal
     * --------+--------
     *   -1    |   0
     *    0    |   0
     *   +1    |  -1
     */
    b->_bal = 0 - !(x->_bal - 1);
    /*
     * x->_bal | prime->_bal
     * --------+--------
     *   -1    |   1
     *    0    |   0
     *   +1    |   0
     */
    prime->_bal = !(x->_bal + 1);
    x->_bal = 0;
  }
  return x;
}


dfk_avltree_hook_t* dfk_avltree_insert(dfk_avltree_t* tree,
    dfk_avltree_hook_t* e)
{
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  DFK_AVLTREE_HOOK_CHECK_INVARIANTS(e);
  assert(e->_left == NULL);
  assert(e->_right == NULL);
  assert(e->_parent == NULL);
  assert(e->_bal == 0);

  if (tree->_root == NULL) {
    tree->_root = e;
    DFK_IF_AVLTREE_CONSTANT_TIME_SIZE(tree->_size = 1);
    return tree->_root;
  }

  {
    dfk_avltree_hook_t* i = tree->_root;
    dfk_avltree_hook_t* prime_parent = NULL;
    dfk_avltree_hook_t* prime = tree->_root;
    int prime_parent_dir = 0;

    while (1) {
      int cmp = tree->_cmp(i, e);
      if (cmp < 0) {
        if (i->_left) {
          if (i->_left->_bal) {
            prime = i->_left;
            prime_parent = i;
            prime_parent_dir = -1;
          }
          i = i->_left;
        } else {
          i->_left = e;
          e->_parent = i;
          break;
        }
      } else if (cmp > 0) {
        if (i->_right) {
          if (i->_right->_bal) {
            prime = i->_right;
            prime_parent = i;
            prime_parent_dir = 1;
          }
          i = i->_right;
        } else {
          i->_right = e;
          e->_parent = i;
          break;
        }
      } else {
        return NULL;
      }
    }

    i = prime;
    while (i != e) {
      int cmp = tree->_cmp(i, e);
      if (cmp < 0) {
        i->_bal -= 1;
        i = i->_left;
      } else  {
        assert(cmp > 0);
        i->_bal += 1;
        i = i->_right;
      }
    }

    {
      dfk_avltree_hook_t* new_prime = NULL;
      if ((prime->_bal == 2 && prime->_right->_bal == -1)
          || (prime->_bal == -2 && prime->_left->_bal == 1)) {
        new_prime = dfk__avltree_double_rot(prime);
      } else if ((prime->_bal == 2 && prime->_right->_bal == 1)
          || (prime->_bal == -2 && prime->_left->_bal == -1)) {
        new_prime = dfk__avltree_single_rot(prime);
      }
      if (new_prime) {
        switch (prime_parent_dir) {
          case -1: {
             prime_parent->_left = new_prime;
             new_prime->_parent = prime_parent;
             break;
          }
          case 0: {
            tree->_root = new_prime;
            new_prime->_parent = NULL;
            break;
          }
          case 1: {
            prime_parent->_right = new_prime;
            new_prime->_parent = prime_parent;
            break;
          }
        }
      }
    }
  }

  DFK_IF_AVLTREE_CONSTANT_TIME_SIZE(tree->_size += 1);
  DFK_AVLTREE_CHECK_INVARIANTS(tree);

  return e;
}


void dfk_avltree_erase(dfk_avltree_t* tree, dfk_avltree_hook_t* e)
{
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  DFK_AVLTREE_HOOK_CHECK_INVARIANTS(e);

  DFK_IF_AVLTREE_CONSTANT_TIME_SIZE(assert(tree->_size));
  DFK_IF_DEBUG(assert(tree == e->_tree));

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
  if (!e->_right) {
    /*
     * Simple case - e has no right subtree.
     * Replace e with it's left subtree.
     * Rebalance P moving upwards.
     *
     *  P─e    ->  P─A
     *    └─A
     */
    newe = e->_left;
    prime = e->_parent;
    erase_direction = prime ? (prime->_left == e ? -1 : 1) : 0;
  } else if (!e->_right->_left) {
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
    dfk_avltree_hook_t* r = e->_right;
    dfk_avltree_hook_t* a = e->_left;
    r->_left = a;
    if (a) {
      a->_parent = r;
    }
    r->_bal = e->_bal;
    /* Since e's right subtree replaces e on it's position,
     * height of R subtree after removal proceudre decreases
     * only if height(R) > heigth(A), or, in terms of nodes' balance, if
     * e->_bal == 1
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
    dfk_avltree_hook_t* r = e->_right;
    dfk_avltree_hook_t* a = e->_left;
    dfk_avltree_hook_t* i = r;
    /* Lookup for X node */
    assert(i->_left);
    while (i->_left->_left) {
      i = i->_left;
    }
    dfk_avltree_hook_t* x = i;
    dfk_avltree_hook_t* y = i->_left;
    assert(y);
    assert(!y->_left);
    dfk_avltree_hook_t* z = y->_right;
    y->_left = e->_left;
    y->_right = e->_right;
    y->_bal = e->_bal;
    r->_parent = y;
    a->_parent = y;
    x->_left = z;
    if (z) {
      z->_parent = x;
    }
    newe = y;
    prime = x;
    erase_direction = -1;
  }

  if (e->_parent) {
    if (e->_parent->_left == e) {
      e->_parent->_left = newe;
    } else {
      e->_parent->_right = newe;
    }
  } else {
    tree->_root = newe;
  }
  if (newe) {
    newe->_parent = e->_parent;
  }

  while (prime) {
    if (erase_direction == -1) {
      /* A node was deleted from the left subtree */
      prime->_bal++;
      /*
       * If prime->_bal equals to 0, no rebalancing is required,
       * but we still need to proceed upwards in tree, because
       * height of the parent subtree might have been changed
       */
      if (prime->_bal == 1) {
        /*
         * Before deletion balance was 0, so left and right subtrees were of the
         * same height. After node deletion height of the left subtree decrased
         * by 1, but overall height of the prime remained the same = height of
         * the right subtree plus 1
         */
        break;
      }
      if (prime->_bal == 2) {
        /*
         * AVL condition is violated - need rotate.
         * After the rotation, height of the tree will decrease by 1
         */
        dfk_avltree_hook_t* parent = prime->_parent;
        int dir = parent ? (parent->_left == prime ? -1 : 1) : 0;
        int rightbal = prime->_right->_bal;
        dfk_avltree_hook_t* new_prime;
        if (rightbal == -1) {
          new_prime = dfk__avltree_double_rot(prime);
        } else {
          new_prime = dfk__avltree_single_rot(prime);
        }
        switch (dir) {
          case -1:
            parent->_left = new_prime;
            new_prime->_parent = parent;
            break;
          case 0:
            tree->_root = new_prime;
            tree->_root->_parent = NULL;
            break;
          case 1:
            parent->_right = new_prime;
            new_prime->_parent = parent;
            break;
        }
        prime = new_prime;
        if (!rightbal) {
          break;
        }
      }
    } else {
      assert(erase_direction == 1);
      prime->_bal--;
      if (prime->_bal == -1) {
        break;
      }
      if (prime->_bal == -2) {
        dfk_avltree_hook_t* parent = prime->_parent;
        int dir = parent ? (parent->_left == prime ? -1 : 1) : 0;
        int leftbal = prime->_left->_bal;
        dfk_avltree_hook_t* new_prime;
        if (leftbal == 1) {
          new_prime = dfk__avltree_double_rot(prime);
        } else {
          new_prime = dfk__avltree_single_rot(prime);
        }
        switch (dir) {
          case -1:
            parent->_left = new_prime;
            new_prime->_parent = parent;
            break;
          case 0:
            tree->_root = new_prime;
            tree->_root->_parent = NULL;
            break;
          case 1:
            parent->_right = new_prime;
            new_prime->_parent = parent;
            break;
        }
        prime = new_prime;
        if (!leftbal) {
          break;
        }
      }
    }
    erase_direction = prime->_parent ? (prime->_parent->_left == prime ? -1 : 1) : 0;
    prime = prime->_parent;
  }

  DFK_IF_AVLTREE_CONSTANT_TIME_SIZE(tree->_size--);
  DFK_IF_DEBUG(e->_tree = NULL);

  DFK_AVLTREE_CHECK_INVARIANTS(tree);
}


dfk_avltree_hook_t* dfk_avltree_find(dfk_avltree_t* tree, void* e,
    dfk_avltree_find_cmp cmp)
{

  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  assert(e);
  assert(cmp);
  {
    dfk_avltree_hook_t* i = tree->_root;
    while (i) {
      int cmpres = cmp(i, e);
      if (cmpres < 0) {
        i = i->_left;
      } else if (cmpres > 0) {
        i = i->_right;
      } else {
        return i;
      }
    }
  }
  return NULL;
}


size_t dfk_avltree_size(dfk_avltree_t* tree)
{
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
#if DFK_AVLTREE_CONSTANT_TIME_SIZE
  return tree->_size;
#else
  size_t size = 0;
  dfk_avltree_it it, end;
  dfk_avltree_begin(tree, &it);
  dfk_avltree_end(tree, &end);
  while (!dfk_avltree_it_equal(&it, &end)) {
    ++size;
    dfk_avltree_it_next(&it);
  }
  return size;
#endif
}


size_t dfk_avltree_sizeof(void)
{
  return sizeof(dfk_avltree_t);
}


void dfk_avltree_begin(dfk_avltree_t* tree, dfk_avltree_it* it)
{
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  assert(it);
  it->value = tree->_root;
  if (it->value) {
    while (it->value->_left) {
      it->value = it->value->_left;
    }
  }
  DFK_IF_DEBUG(it->_tree = tree);
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  DFK_AVLTREE_IT_CHECK_INVARIANTS(it);
}


void dfk_avltree_end(dfk_avltree_t* tree, dfk_avltree_it* it)
{
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  DFK_AVLTREE_IT_CHECK_INVARIANTS(it);
  it->value = NULL;
  DFK_IF_DEBUG(it->_tree = tree);
  DFK_AVLTREE_CHECK_INVARIANTS(tree);
  DFK_AVLTREE_IT_CHECK_INVARIANTS(it);
}


void dfk_avltree_it_next(dfk_avltree_it* it)
{
  DFK_AVLTREE_IT_CHECK_INVARIANTS(it);
  assert(it->value); /* can not increment end iterator */
  if (it->value->_right) {
    it->value = it->value->_right;
    while (it->value->_left) {
      it->value = it->value->_left;
    }
    return;
  }
  while (it->value->_parent && it->value->_parent->_right == it->value) {
    it->value = it->value->_parent;
  }
  it->value = it->value->_parent;
}


int dfk_avltree_it_equal(dfk_avltree_it* lhs, dfk_avltree_it* rhs)
{
  DFK_AVLTREE_IT_CHECK_INVARIANTS(lhs);
  DFK_AVLTREE_IT_CHECK_INVARIANTS(rhs);
  DFK_IF_DEBUG(assert(lhs->_tree == rhs->_tree));
  return lhs->value == rhs->value;
}


size_t dfk_avltree_it_sizeof(void)
{
  return sizeof(dfk_avltree_it);
}

