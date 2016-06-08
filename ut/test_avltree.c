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

#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/avltree.h>
#include "ut.h"


typedef struct node_t {
  dfk_avltree_hook_t hook;
  int value;
} node_t;


static int node_cmp(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r)
{
  if (((node_t*) r)->value < ((node_t*) l)->value) {
    return -1;
  } else if (((node_t*) l)->value == ((node_t*) r)->value) {
    return 0;
  } else {
    return 1;
  }
}


typedef struct tree_fixture_t {
  dfk_t dfk;
  dfk_avltree_t tree;
  node_t* nodes;
  node_t new_node;
} tree_fixture_t;


static void tree_fixture_setup(tree_fixture_t* f)
{
  dfk_init(&f->dfk);
  f->nodes = NULL;
  dfk_avltree_hook_init((dfk_avltree_hook_t*) &f->new_node);
  dfk_avltree_init(&f->tree, node_cmp);
}


static void tree_fixture_teardown(tree_fixture_t* f)
{
  dfk_avltree_free(&f->tree);
  dfk_avltree_hook_free((dfk_avltree_hook_t*) &f->new_node);
  if (f->nodes) {
    DFK_FREE(&f->dfk, f->nodes);
  }
  dfk_free(&f->dfk);
}


/**
 * Initiale tree_fixture_t .tree and .nodes members with the tree
 * described by `definition'.
 *
 * Definition string consists of the following lines:
 * <node value> <node balance> <left child index> <right child index>\n
 * where
 *  - <node value>: integer value of the node
 *  - <node balance>: 0, -1, or +1. Zero means that left and right subtrees
 *    of the given node have the same height, minus one - height of the left
 *    subtree is larger than the right's, plus one - otherwise.
 *  - <left child index>, <right child index>: indices of the child nodes
 *    in the list, -1 if there is no
 */
static void ut_parse_avltree(tree_fixture_t* f, const char* definition)
{
  size_t nnodes = 0;
  {
    size_t i;
    for (i = 0; i < strlen(definition); ++i) {
      if (definition[i] == '\n') {
        ++nnodes;
      }
    }
  }
  f->nodes = DFK_MALLOC(&f->dfk, nnodes * sizeof(node_t));
  {
    const char* p = definition;
    const char* end = definition + strlen(definition);
    size_t i = 0;
    while (p < end) {
      int lindex, rindex;
      int scanned = sscanf(p, "%d %hhd %d %d\n",
          &f->nodes[i].value, &f->nodes[i].hook.bal, &lindex, &rindex);
      EXPECT(scanned == 4);
      if (lindex < 0) {
        f->nodes[i].hook.left = NULL;
      } else {
        f->nodes[i].hook.left = (dfk_avltree_hook_t*) (f->nodes + lindex);
      }
      if (rindex < 0) {
        f->nodes[i].hook.right = NULL;
      } else {
        f->nodes[i].hook.right = (dfk_avltree_hook_t*) (f->nodes + rindex);
      }
      while ((p < end) && (*p != '\n')) {
        ++p;
      }
      ++p;
      ++i;
    }
  }
  f->tree.root = (dfk_avltree_hook_t*) (f->nodes + nnodes - 1);
}


TEST_F(tree_fixture, avltree, insert_double_left_rotation)
{
  /*
   *                                      ┌─23
   *         ┌─23                      ┌─18
   *      ┌─18                         │  └─17
   *      │  └─16                   ┌─16
   *   ┌─15        --insert 17-->   │  └─15
   *   │  └─11                      │     └─11
   *  10                           10
   *   └─5                          └─5
   *     └─1                          └─1
   */

  ut_parse_avltree(fixture,
    /* 0 */ "23  0 -1 -1\n"
    /* 1 */ "18  0  2  0\n"
    /* 2 */ "16  0 -1 -1\n"
    /* 3 */ "15  1  4  1\n"
    /* 4 */ "11  0 -1 -1\n"
    /* 5 */ " 5 -1  6 -1\n"
    /* 6 */ " 1  0 -1 -1\n"
    /* 7 */ "10  1  5  3\n"
  );

  fixture->new_node.value = 17;
  dfk_avltree_insert(&fixture->tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(((node_t*) fixture->tree.root->right)->value == 16);
  EXPECT(((node_t*) fixture->tree.root->right->right)->value == 18);
  EXPECT(((node_t*) fixture->tree.root->right->right->right)->value == 23);
  EXPECT(((node_t*) fixture->tree.root->right->right->left)->value == 17);
  EXPECT(((node_t*) fixture->tree.root->right->left)->value == 15);
}

