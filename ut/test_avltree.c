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
  dfk_avltree_t in_tree;
  node_t* in_nodes;
  node_t new_node;
  dfk_avltree_t expected_tree;
  node_t* expected_nodes;
} tree_fixture_t;


static void tree_fixture_setup(tree_fixture_t* f)
{
  dfk_init(&f->dfk);
  f->in_nodes = NULL;
  dfk_avltree_init(&f->in_tree, node_cmp);
  f->expected_nodes = NULL;
  dfk_avltree_init(&f->expected_tree, node_cmp);
  dfk_avltree_hook_init((dfk_avltree_hook_t*) &f->new_node);
}


static void tree_fixture_teardown(tree_fixture_t* f)
{
  dfk_avltree_free(&f->in_tree);
  dfk_avltree_hook_free((dfk_avltree_hook_t*) &f->new_node);
  if (f->in_nodes) {
    DFK_FREE(&f->dfk, f->in_nodes);
  }
  dfk_avltree_free(&f->expected_tree);
  if (f->expected_nodes) {
    DFK_FREE(&f->dfk, f->expected_nodes);
  }
  dfk_free(&f->dfk);
}


/**
 * Initialize dfk_avltree_t struct with the tree described by `definition'.
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
 *
 * Returns a pointer to an array of tree nodes.
 */
static node_t* ut_parse_avltree(dfk_t* dfk, dfk_avltree_t* tree, const char* definition)
{
  size_t nnodes = 0;
  node_t* nodes;
  {
    size_t i;
    for (i = 0; i < strlen(definition); ++i) {
      if (definition[i] == '\n') {
        ++nnodes;
      }
    }
  }
  nodes = DFK_MALLOC(dfk, nnodes * sizeof(node_t));
  {
    size_t i;
    for (i = 0; i < nnodes; ++i) {
      nodes[i].hook.parent = NULL;
    }
  }
  {
    const char* p = definition;
    const char* end = definition + strlen(definition);
    size_t i = 0;
    while (p < end) {
      int lindex, rindex;
      int scanned = sscanf(p, "%d %hhd %d %d\n",
          &nodes[i].value, &nodes[i].hook.bal, &lindex, &rindex);
      EXPECT(scanned == 4);
      if (lindex < 0) {
        nodes[i].hook.left = NULL;
      } else {
        nodes[i].hook.left = (dfk_avltree_hook_t*) (nodes + lindex);
        ((dfk_avltree_hook_t*) (nodes + lindex))->parent = &nodes[i].hook;
      }
      if (rindex < 0) {
        nodes[i].hook.right = NULL;
      } else {
        nodes[i].hook.right = (dfk_avltree_hook_t*) (nodes + rindex);
        ((dfk_avltree_hook_t*) (nodes + rindex))->parent = &nodes[i].hook;
      }
      while ((p < end) && (*p != '\n')) {
        ++p;
      }
      ++p;
      ++i;
    }
  }
  tree->root = (dfk_avltree_hook_t*) (nodes + nnodes - 1);
  tree->root->parent = NULL;
  return nodes;
}


static int ut_trees_equal(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r, dfk_avltree_cmp cmp)
{
  if (!l && !r) {
    return 1;
  }

  return l && r
      && (cmp(l, r) == 0)
      && ut_trees_equal(l->left, r->left, cmp)
      && ut_trees_equal(l->right, r->right, cmp);
}


TEST_F(tree_fixture, avltree, stackoverflow_3955680_insert_1a)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *                        ┌─20
   *  20   --insert 15-->  15
   *   └─4                  └─4
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ " 4  0 -1 -1\n"
      /* 1 */ "20 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "20  0 -1 -1\n"
      /* 1 */ " 4  0 -1 -1\n"
      /* 2 */ "15  0  1  0\n"
  );

  fixture->new_node.value = 15;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, stackoverflow_3955680_insert_2a)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *                            ┌─26
   *   ┌─26                  ┌─20
   *  20                     │  └─15
   *   │ ┌─9  --insert 15->  9
   *   └─4                   └─4
   *     └─3                   └─3
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ " 3  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ "26  0 -1 -1\n"
      /* 3 */ " 4  0  0  1\n"
      /* 4 */ "20 -1  3  2\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ "15  0 -1 -1\n"
      /* 2 */ "20  0  1  0\n"
      /* 3 */ " 3  0 -1 -1\n"
      /* 4 */ " 4 -1  3 -1\n"
      /* 5 */ " 9  0  4  2\n"
  );

  fixture->new_node.value = 15;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, stackoverflow_3955680_insert_3a)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *      ┌─30                        ┌─30
   *   ┌─26                        ┌─26
   *   │  └─21                     │  └─21
   *  20                        ┌─20
   *   │   ┌─11                 │  │  ┌─15
   *   │ ┌─9     --insert 15->  │  └─11
   *   │ │ └─7                  9
   *   └─4                      │ ┌─7
   *     └─3                    └─4
   *       └─2                    └─3
   *                                └─2
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "30  0 -1 -1\n"
      /* 1 */ "21  0 -1 -1\n"
      /* 2 */ "26  0  1  0\n"
      /* 3 */ "11  0 -1 -1\n"
      /* 4 */ " 7  0 -1 -1\n"
      /* 5 */ " 9  0  4  3\n"
      /* 6 */ " 2  0 -1 -1\n"
      /* 7 */ " 3 -1  6 -1\n"
      /* 8 */ " 4  0  7  5\n"
      /* 9 */ "20 -1  8  2\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /*  0 */ "30  0 -1 -1\n"
      /*  1 */ "21  0 -1 -1\n"
      /*  2 */ "26  0  1  0\n"
      /*  3 */ "15  0 -1 -1\n"
      /*  4 */ "11  1 -1  3\n"
      /*  5 */ "20  0  4  2\n"
      /*  6 */ " 7  0 -1 -1\n"
      /*  7 */ " 2  0 -1 -1\n"
      /*  8 */ " 3 -1  7 -1\n"
      /*  9 */ " 4 -1  8  6\n"
      /* 10 */ " 9  0  9  5\n"
  );

  fixture->new_node.value = 15;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, stackoverflow_3955680_insert_1b)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *                        ┌─20
   *   20    --insert 8->  8
   *    └─4                 └─4
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ " 4  0 -1 -1\n"
      /* 1 */ "20 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "20  0 -1 -1\n"
      /* 1 */ " 4  0 -1 -1\n"
      /* 2 */ " 8  0  1  0\n"
  );

  fixture->new_node.value = 8;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, stackoverflow_3955680_insert_2b)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *    ┌─26                     ┌─26
   *   20                     ┌─20
   *    │ ┌─9   --insert 8->  9
   *    └─4                   │ ┌─8
   *      └─3                 └─4
   *                            └─3
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ " 3  0 -1 -1\n"
      /* 3 */ " 4  0  2  1\n"
      /* 4 */ "20 -1  3  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ "20  1 -1  0\n"
      /* 2 */ " 8  0 -1 -1\n"
      /* 3 */ " 3  0 -1 -1\n"
      /* 4 */ " 4  0  3  2\n"
      /* 5 */ " 9  0  4  1\n"
  );

  fixture->new_node.value = 8;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, stackoverflow_3955680_insert_3b)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *       ┌─30                       ┌─30
   *    ┌─26                       ┌─26
   *    │  └─21                    │  └─21
   *   20                       ┌─20
   *    │   ┌─11                │  └─11
   *    │ ┌─9     --insert 8->  9
   *    │ │ └─7                 │   ┌─8
   *    └─4                     │ ┌─7
   *      └─3                   └─4
   *        └─2                   └─3
   *                                └─2
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "30  0 -1 -1\n"
      /* 1 */ "21  0 -1 -1\n"
      /* 2 */ "26  0  1  0\n"
      /* 3 */ "11  0 -1 -1\n"
      /* 4 */ " 7  0 -1 -1\n"
      /* 5 */ " 9  0  4  3\n"
      /* 6 */ " 2  0 -1 -1\n"
      /* 7 */ " 3 -1  6 -1\n"
      /* 8 */ " 4  0  7  5\n"
      /* 9 */ "20 -1  8  2\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /*  0 */ "30  0 -1 -1\n"
      /*  1 */ "21  0 -1 -1\n"
      /*  2 */ "26  0  1  0\n"
      /*  3 */ "11  0 -1 -1\n"
      /*  4 */ "20  1  3  2\n"
      /*  5 */ " 8  0 -1 -1\n"
      /*  6 */ " 7  1 -1  5\n"
      /*  7 */ " 2  0 -1 -1\n"
      /*  8 */ " 3 -1  7 -1\n"
      /*  9 */ " 4  0  8  6\n"
      /* 10 */ " 9  0  9  4\n"
  );

  fixture->new_node.value = 8;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "23  0 -1 -1\n"
      /* 1 */ "18  0  2  0\n"
      /* 2 */ "16  0 -1 -1\n"
      /* 3 */ "15  1  4  1\n"
      /* 4 */ "11  0 -1 -1\n"
      /* 5 */ " 5 -1  6 -1\n"
      /* 6 */ " 1  0 -1 -1\n"
      /* 7 */ "10  1  5  3\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "23  0 -1 -1\n"
      /* 1 */ "17  0 -1 -1\n"
      /* 2 */ "18  0  1  0\n"
      /* 3 */ "11  0 -1 -1\n"
      /* 4 */ "15 -1  3 -1\n"
      /* 5 */ "16  0  4  2\n"
      /* 6 */ " 1  0 -1 -1\n"
      /* 7 */ " 5 -1  6 -1\n"
      /* 8 */ "10  1  7  5\n"
  );

  fixture->new_node.value = 17;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_double_right_rotation)
{
  /*
   *      ┌─21                      ┌─21
   *   ┌─16                      ┌─16
   *  15                        15
   *   │  ┌─12                   │    ┌─12
   *   └─10      --insert 9-->   │ ┌─10
   *      │ ┌─8                  │ │  └─9
   *      └─5                    └─8
   *        └─3                    └─5
   *                                 └─3
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "21  0 -1 -1\n"
      /* 1 */ "16  1 -1  0\n"
      /* 2 */ "12  0 -1 -1\n"
      /* 3 */ " 8  0 -1 -1\n"
      /* 4 */ " 3  0 -1 -1\n"
      /* 5 */ " 5  0  4  3\n"
      /* 6 */ "10 -1  5  2\n"
      /* 7 */ "15 -1  6  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "21  0 -1 -1\n"
      /* 1 */ "16  1 -1  0\n"
      /* 2 */ "12  0 -1 -1\n"
      /* 3 */ " 9  0 -1 -1\n"
      /* 4 */ "10  0  3  2\n"
      /* 5 */ " 3  0 -1 -1\n"
      /* 6 */ " 5 -1  5 -1\n"
      /* 7 */ " 8  0  6  4\n"
      /* 8 */ "15 -1  7  1\n"
  );

  fixture->new_node.value = 9;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_single_left_rotation)
{
  /*
   *   ┌─15                   ┌─17
   *  10     --insert 17-->  15
   *                          └─10
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 1 */ "10  1 -1  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "17  0 -1 -1\n"
      /* 1 */ "10  0 -1 -1\n"
      /* 2 */ "15  0  1  0\n"
  );

  fixture->new_node.value = 17;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_single_right_rotation)
{
  /*
   *  15                     ┌─15
   *   └─10  --insert 7-->  10
   *                         └─7
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "10  0 -1 -1\n"
      /* 1 */ "15 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 1 */ " 7  0 -1 -1\n"
      /* 2 */ "10  0  1  0\n"
  );

  fixture->new_node.value = 7;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_no_rotation)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *   ┌─26                  ┌─26
   *  20                    20
   *   │ ┌─9  --insert 3->   │ ┌─9
   *   └─4                   └─4
   *                           └─3
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ " 4  1 -1  1\n"
      /* 3 */ "20 -1  2  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->dfk, &fixture->expected_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ " 3  0 -1 -1\n"
      /* 3 */ " 4  0  2  1\n"
      /* 4 */ "20 -1  3  0\n"
  );

  fixture->new_node.value = 3;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree.root, fixture->expected_tree.root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_existing)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *    ┌─26
   *   20
   *    │ ┌─9
   *    └─4
   *      └─3
   *
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ " 3  0 -1 -1\n"
      /* 3 */ " 4  0  2  1\n"
      /* 4 */ "20 -1  3  0\n"
  );

  {
    int existing[] = {20, 26, 9, 4, 3};
    size_t i;
    for (i = 0; i < DFK_SIZE(existing); ++i) {
      dfk_avltree_hook_t* res;
      fixture->new_node.value = existing[i];
      res = dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
      EXPECT(!res);
    }
  }
}


TEST(avltree, insert_empty)
{
  dfk_avltree_t tree;
  node_t node;

  dfk_avltree_init(&tree, node_cmp);
  dfk_avltree_hook_init(&node.hook);
  node.value = 10;
  dfk_avltree_insert(&tree, (dfk_avltree_hook_t*) &node);
  ASSERT(tree.root == (dfk_avltree_hook_t*) &node);
  ASSERT(tree.root->bal == 0);
  ASSERT(tree.root->left == NULL);
  ASSERT(tree.root->right == NULL);
}


static int node_int_lookup_cmp(dfk_avltree_hook_t* hook, void* v)
{
  int* value = (int*) v;
  node_t* node = (node_t*) hook;
  if (*value < node->value) {
    return -1;
  } else if (*value > node->value) {
    return 1;
  } else {
    return 0;
  }
}


TEST_F(tree_fixture, avltree, lookup)
{
  /*
   *
   *         ┌─23
   *      ┌─18
   *      │  └─16
   *   ┌─15
   *   │  └─11
   *  10
   *   └─5
   *     └─1
   */
  int query[] = {10, 15, 11, 16, 23, 18, 1};

  fixture->in_nodes = ut_parse_avltree(&fixture->dfk, &fixture->in_tree,
      /* 0 */ "23  0 -1 -1\n"
      /* 1 */ "18  0  2  0\n"
      /* 2 */ "16  0 -1 -1\n"
      /* 3 */ "15  1  4  1\n"
      /* 4 */ "11  0 -1 -1\n"
      /* 5 */ " 5 -1  6 -1\n"
      /* 6 */ " 1  0 -1 -1\n"
      /* 7 */ "10  1  5  3\n"
  );

  {
    size_t i;
    for (i = 0; i < DFK_SIZE(query); ++i) {
      dfk_avltree_hook_t* res = dfk_avltree_lookup(
          &fixture->in_tree, query + i, node_int_lookup_cmp);
      ASSERT(res);
      EXPECT(((node_t*) res)->value == query[i]);
    }
  }
  {
    int non_existent = 12;
    EXPECT(!dfk_avltree_lookup(&fixture->in_tree, &non_existent, node_int_lookup_cmp));
    non_existent = -1;
    EXPECT(!dfk_avltree_lookup(&fixture->in_tree, &non_existent, node_int_lookup_cmp));
  }
}


