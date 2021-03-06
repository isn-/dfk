/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdlib.h>
#include <dfk/internal.h>
#include <dfk/avltree.h>
#include <ut.h>


TEST(avltree, sizeof)
{
  EXPECT(dfk_avltree_sizeof() == sizeof(dfk_avltree_t));
}


TEST(avltree, iterator_sizeof)
{
  EXPECT(dfk_avltree_it_sizeof() == sizeof(dfk_avltree_it));
}


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
  dfk_avltree_t in_tree;
  node_t* in_nodes;
  node_t new_node;
  int erase_value;
  dfk_avltree_t expected_tree;
  node_t* expected_nodes;
} tree_fixture_t;


static void tree_fixture_setup(tree_fixture_t* f)
{
  f->in_nodes = NULL;
  dfk_avltree_init(&f->in_tree, node_cmp);
  f->expected_nodes = NULL;
  dfk_avltree_init(&f->expected_tree, node_cmp);
  dfk_avltree_hook_init((dfk_avltree_hook_t*) &f->new_node);
}


static void tree_fixture_teardown(tree_fixture_t* f)
{
  if (f->in_nodes) {
    free(f->in_nodes);
  }
  if (f->expected_nodes) {
    free(f->expected_nodes);
  }
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
static node_t* ut_parse_avltree(dfk_avltree_t* tree, const char* definition)
{
  size_t nnodes = 0;
  node_t* nodes;

  for (size_t i = 0; i < strlen(definition); ++i) {
    if (definition[i] == '\n') {
      ++nnodes;
    }
  }

  if (!nnodes) {
#if DFK_AVLTREE_CONSTANT_TIME_SIZE
    tree->_size = 0;
#endif
    tree->_root = NULL;
    return NULL;
  }

  nodes = malloc(nnodes * sizeof(node_t));
  EXPECT(nodes);
  for (size_t i = 0; i < nnodes; ++i) {
    nodes[i].hook._parent = NULL;
    DFK_IF_DEBUG(nodes[i].hook._tree = tree);
  }
  {
    const char* p = definition;
    const char* end = definition + strlen(definition);
    size_t i = 0;
    while (p < end) {
      int lindex, rindex;
      int scanned = sscanf(p, "%d %hhd %d %d\n",
          &nodes[i].value, &nodes[i].hook._bal, &lindex, &rindex);
      EXPECT(scanned == 4);
      if (lindex < 0) {
        nodes[i].hook._left = NULL;
      } else {
        nodes[i].hook._left = (dfk_avltree_hook_t*) (nodes + lindex);
        ((dfk_avltree_hook_t*) (nodes + lindex))->_parent = &nodes[i].hook;
      }
      if (rindex < 0) {
        nodes[i].hook._right = NULL;
      } else {
        nodes[i].hook._right = (dfk_avltree_hook_t*) (nodes + rindex);
        ((dfk_avltree_hook_t*) (nodes + rindex))->_parent = &nodes[i].hook;
      }
      while ((p < end) && (*p != '\n')) {
        ++p;
      }
      ++p;
      ++i;
    }
  }
  tree->_root = (dfk_avltree_hook_t*) (nodes + nnodes - 1);
  tree->_root->_parent = NULL;
#if DFK_AVLTREE_CONSTANT_TIME_SIZE
  tree->_size = nnodes;
#endif
  return nodes;
}


static int ut_trees_equal(dfk_avltree_hook_t* l, dfk_avltree_hook_t* r,
    dfk_avltree_cmp cmp)
{
  if (!l && !r) {
    return 1;
  }

  return l && r
      && (cmp(l, r) == 0)
      && ut_trees_equal(l->_left, r->_left, cmp)
      && ut_trees_equal(l->_right, r->_right, cmp);
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ " 4  0 -1 -1\n"
      /* 1 */ "20 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "20  0 -1 -1\n"
      /* 1 */ " 4  0 -1 -1\n"
      /* 2 */ "15  0  1  0\n"
  );

  fixture->new_node.value = 15;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ " 3  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ "26  0 -1 -1\n"
      /* 3 */ " 4  0  0  1\n"
      /* 4 */ "20 -1  3  2\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ "15  0 -1 -1\n"
      /* 2 */ "20  0  1  0\n"
      /* 3 */ " 3  0 -1 -1\n"
      /* 4 */ " 4 -1  3 -1\n"
      /* 5 */ " 9  0  4  2\n"
  );

  fixture->new_node.value = 15;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
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

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
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
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, stackoverflow_3955680_insert_1b)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *                       ┌─20
   *   20    --insert 8->  8
   *    └─4                └─4
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ " 4  0 -1 -1\n"
      /* 1 */ "20 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "20  0 -1 -1\n"
      /* 1 */ " 4  0 -1 -1\n"
      /* 2 */ " 8  0  1  0\n"
  );

  fixture->new_node.value = 8;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ " 3  0 -1 -1\n"
      /* 3 */ " 4  0  2  1\n"
      /* 4 */ "20 -1  3  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ "20  1 -1  0\n"
      /* 2 */ " 8  0 -1 -1\n"
      /* 3 */ " 3  0 -1 -1\n"
      /* 4 */ " 4  0  3  2\n"
      /* 5 */ " 9  0  4  1\n"
  );

  fixture->new_node.value = 8;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
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

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
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
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "23  0 -1 -1\n"
      /* 1 */ "18  0  2  0\n"
      /* 2 */ "16  0 -1 -1\n"
      /* 3 */ "15  1  4  1\n"
      /* 4 */ "11  0 -1 -1\n"
      /* 5 */ " 5 -1  6 -1\n"
      /* 6 */ " 1  0 -1 -1\n"
      /* 7 */ "10  1  5  3\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
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
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_double_left_rotation_with_child)
{
  /*
   *                               ┌─10
   *        ┌─10                 ┌─9
   *      ┌─9                  ┌─8
   *    ┌─8                    │ │ ┌─7
   *    │ │ ┌─6                │ └─6
   *    │ └─5    --insert 7--> 5
   *    │   └─4                │ ┌─4
   *    3                      └─3
   *    └─2                      └─2
   *      └─1                      └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "10  0 -1 -1\n"
      /* 1 */ " 9  1 -1  0\n"
      /* 2 */ " 6  0 -1 -1\n"
      /* 3 */ " 4  0 -1 -1\n"
      /* 4 */ " 5  0  3  2\n"
      /* 5 */ " 8  0  4  1\n"
      /* 6 */ " 1  0 -1 -1\n"
      /* 7 */ " 2 -1  6 -1\n"
      /* 8 */ " 3  1  7  5\n"

  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "10  0 -1 -1\n"
      /* 1 */ " 9  1 -1  0\n"
      /* 2 */ " 7  0 -1 -1\n"
      /* 3 */ " 6  1 -1  2\n"
      /* 4 */ " 8  0  3  1\n"
      /* 5 */ " 4  0 -1 -1\n"
      /* 6 */ " 1  0 -1 -1\n"
      /* 7 */ " 2 -1  6 -1\n"
      /* 8 */ " 3 -1  7  5\n"
      /* 9 */ " 5  0  8  4\n"
  );

  fixture->new_node.value = 7;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "21  0 -1 -1\n"
      /* 1 */ "16  1 -1  0\n"
      /* 2 */ "12  0 -1 -1\n"
      /* 3 */ " 8  0 -1 -1\n"
      /* 4 */ " 3  0 -1 -1\n"
      /* 5 */ " 5  0  4  3\n"
      /* 6 */ "10 -1  5  2\n"
      /* 7 */ "15 -1  6  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
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
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_single_left_rotation)
{
  /*
   *   ┌─15                   ┌─17
   *  10     --insert 17-->  15
   *                          └─10
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 1 */ "10  1 -1  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "17  0 -1 -1\n"
      /* 1 */ "10  0 -1 -1\n"
      /* 2 */ "15  0  1  0\n"
  );

  fixture->new_node.value = 17;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_single_left_rotation_with_child)
{
  /*
   *                          ┌─6
   *    ┌─5                 ┌─5
   *  ┌─4                   4
   *  │ └─3  --insert 6-->  │ ┌─3
   *  2                     └─2
   *  └─1                     └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ " 5  0 -1 -1\n"
      /* 1 */ " 3  0 -1 -1\n"
      /* 2 */ " 4  0  1  0\n"
      /* 3 */ " 1  0 -1 -1\n"
      /* 4 */ " 2  1  3  2\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ " 6  0 -1 -1\n"
      /* 1 */ " 5  1 -1  0\n"
      /* 2 */ " 1  0 -1 -1\n"
      /* 3 */ " 3  0 -1 -1\n"
      /* 4 */ " 2  0  2  3\n"
      /* 5 */ " 4  0  4  1\n"
  );

  fixture->new_node.value = 6;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_single_right_rotation)
{
  /*
   *  15                     ┌─15
   *   └─10  --insert 7-->  10
   *                         └─7
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "10  0 -1 -1\n"
      /* 1 */ "15 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 1 */ " 7  0 -1 -1\n"
      /* 2 */ "10  0  1  0\n"
  );

  fixture->new_node.value = 7;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_single_right_rotation_with_child)
{
  /*
   * ┌─6                     ┌─6
   * 5                     ┌─5
   * │ ┌─4                 │ └─4
   * └─3    --insert 1-->  3
   *   └─2                 └─2
   *                         └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ " 6  0 -1 -1\n"
      /* 1 */ " 4  0 -1 -1\n"
      /* 2 */ " 2  0 -1 -1\n"
      /* 3 */ " 3  0  2  1\n"
      /* 4 */ " 5 -1  3  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ " 6  0 -1 -1\n"
      /* 1 */ " 4  0 -1 -1\n"
      /* 2 */ " 5  0  1  0\n"
      /* 3 */ " 1  0 -1 -1\n"
      /* 4 */ " 2 -1  3 -1\n"
      /* 5 */ " 3  0  4  2\n"
  );

  fixture->new_node.value = 1;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ " 4  1 -1  1\n"
      /* 3 */ "20 -1  2  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "26  0 -1 -1\n"
      /* 1 */ " 9  0 -1 -1\n"
      /* 2 */ " 3  0 -1 -1\n"
      /* 3 */ " 4  0  2  1\n"
      /* 4 */ "20 -1  3  0\n"
  );

  fixture->new_node.value = 3;
  dfk_avltree_insert(&fixture->in_tree, (dfk_avltree_hook_t*) &fixture->new_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, insert_existing)
{
  /*
   * Based on http://stackoverflow.com/a/13843966
   *
   *   ┌─26
   *  20
   *   │ ┌─9
   *   └─4
   *     └─3
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
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
  EXPECT(tree._root == (dfk_avltree_hook_t*) &node);
  EXPECT(tree._root->_bal == 0);
  EXPECT(tree._root->_left == NULL);
  EXPECT(tree._root->_right == NULL);
}


TEST_F(tree_fixture, avltree, insert_algcourse_cs_msu_su)
{
  /* Based on algcourse.cs.msu.su:
   * http://algcourse.cs.msu.su/wp-content/uploads/2011/09/Lection19.pdf
   * page 17
   *
   * Insertion of keys 4, 5, 7, 2, 1, 3, 6 into an empty tree should yield:
   *
   *    ┌─7
   *  ┌─6
   *  │ └─5
   *  4
   *  │ ┌─3
   *  └─2
   *    └─1
   */
  dfk_avltree_t tree;
  int values[] = {4, 5, 7, 2, 1, 3, 6};
  node_t nodes[DFK_SIZE(values)];
  size_t i;

  dfk_avltree_init(&tree, node_cmp);
  for (i = 0; i < DFK_SIZE(nodes); ++i) {
    dfk_avltree_hook_init(&nodes[i].hook);
    nodes[i].value = values[i];
  }
  for (i = 0; i < DFK_SIZE(nodes); ++i) {
    dfk_avltree_insert(&tree, (dfk_avltree_hook_t*) &nodes[i]);
  }

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "5  0 -1 -1\n"
      /* 2 */ "6  0  1  0\n"
      /* 3 */ "3  0 -1 -1\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2  0  4  3\n"
      /* 6 */ "4  0  5  2\n"
  );

  EXPECT(ut_trees_equal(tree._root, fixture->expected_tree._root, node_cmp));
}


static int node_int_find_cmp(dfk_avltree_hook_t* hook, void* v)
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

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
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
      dfk_avltree_hook_t* res = dfk_avltree_find(
          &fixture->in_tree, query + i, node_int_find_cmp);
      EXPECT(res);
      EXPECT(((node_t*) res)->value == query[i]);
    }
  }
  {
    int non_existent = 12;
    EXPECT(!dfk_avltree_find(&fixture->in_tree, &non_existent, node_int_find_cmp));
    non_existent = -1;
    EXPECT(!dfk_avltree_find(&fixture->in_tree, &non_existent, node_int_find_cmp));
  }
}


typedef struct tree_traversal_fixture_t {
  dfk_t dfk;
  dfk_avltree_t tree;
  node_t* nodes;
} tree_traversal_fixture_t;


static void tree_traversal_fixture_setup(tree_traversal_fixture_t* f)
{
  f->nodes = NULL;
  dfk_avltree_init(&f->tree, node_cmp);
}


static void tree_traversal_fixture_teardown(tree_traversal_fixture_t* f)
{
  if (f->nodes) {
    free(f->nodes);
  }
}


TEST_F(tree_traversal_fixture, avltree, traversal_empty)
{
  dfk_avltree_it begin, end;
  dfk_avltree_begin(&fixture->tree, &begin);
  dfk_avltree_end(&fixture->tree, &end);
  EXPECT(dfk_avltree_it_equal(&begin, &end));
}


TEST_F(tree_traversal_fixture, avltree, traversal_complete)
{
  /*
   *       ┌─15
   *    ┌─14
   *    │  └─13
   * ┌─12
   * │  │  ┌─11
   * │  └─10
   * │     └─9
   * 8
   * │   ┌─7
   * │ ┌─6
   * │ │ └─5
   * └─4
   *   │ ┌─3
   *   └─2
   *     └─1
   */
  fixture->nodes = ut_parse_avltree(&fixture->tree,
      /*  0 */ " 1  0 -1 -1\n"
      /*  1 */ " 3  0 -1 -1\n"
      /*  2 */ " 2  0  0  1\n"
      /*  3 */ " 5  0 -1 -1\n"
      /*  4 */ " 7  0 -1 -1\n"
      /*  5 */ " 6  0  3  4\n"
      /*  6 */ " 4  0  2  5\n"
      /*  7 */ " 9  0 -1 -1\n"
      /*  8 */ "11  0 -1 -1\n"
      /*  9 */ "10  0  7  8\n"
      /* 10 */ "13  0 -1 -1\n"
      /* 11 */ "15  0 -1 -1\n"
      /* 12 */ "14  0 10 11\n"
      /* 13 */ "12  0  9 12\n"
      /* 14 */ " 8  0  6 13\n"
  );
  dfk_avltree_it it, end;
  dfk_avltree_begin(&fixture->tree, &it);
  dfk_avltree_end(&fixture->tree, &end);
  int i = 1;
  for (; i < 20 && !dfk_avltree_it_equal(&it, &end); ++i) {
    EXPECT(((node_t*) it.value)->value == i);
    dfk_avltree_it_next(&it);
  }
  EXPECT(dfk_avltree_it_equal(&it, &end));
  EXPECT(i == 16);
}


TEST_F(tree_traversal_fixture, avltree, traversal_1k)
{
  size_t nnodes = 1000;
  fixture->nodes = malloc(nnodes * sizeof(node_t));
  for (size_t i = 0; i < nnodes; ++i) {
    dfk_avltree_hook_init((dfk_avltree_hook_t*) (fixture->nodes + i));
    fixture->nodes[i].value = (223 * i + 1046527) % 16769023;
    dfk_avltree_insert(&fixture->tree,
        (dfk_avltree_hook_t*) (fixture->nodes + i));
  }
  dfk_avltree_it it, end;
  dfk_avltree_begin(&fixture->tree, &it);
  dfk_avltree_end(&fixture->tree, &end);
  size_t i = 1;
  int prev_value = ((node_t*) it.value)->value;
  for (; i < (nnodes + 2) && !dfk_avltree_it_equal(&it, &end); ++i) {
    int current_value = ((node_t*) it.value)->value;
    dfk_avltree_it_next(&it);
    EXPECT(current_value >= prev_value);
    prev_value = current_value;
  }
  EXPECT(dfk_avltree_it_equal(&it, &end));
  EXPECT(i == (nnodes + 1));
}


TEST_F(tree_fixture, avltree, erase_last)
{
  /*
   *  10  --erase 10-->  *
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "10  0 -1 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree, "");

  int erase_value = 10;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_two_nodes_left_child)
{
  /*
   *  10    --erase 1-->  10
   *   └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ " 1  0 -1 -1\n"
      /* 1 */ "10 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "10  0 -1 -1\n"
  );

  int erase_value = 1;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_two_nodes_right_child)
{
  /*
   *   ┌─11  --erase 11-->  10
   *  10
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "11  0 -1 -1\n"
      /* 1 */ "10  1 -1  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "10  0 -1 -1\n"
  );

  int erase_value = 11;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_two_nodes_left_root)
{
  /*
   *  10    --erase 10-->  1
   *   └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ " 1  0 -1 -1\n"
      /* 1 */ "10 -1  0 -1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ " 1  0 -1 -1\n"
  );

  int erase_value = 10;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_two_nodes_right_root)
{
  /*
   *   ┌─11  --erase 10-->  11
   *  10
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "11  0 -1 -1\n"
      /* 1 */ "10  1 -1  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "11  0 -1 -1\n"
  );

  int erase_value = 10;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_three_nodes_left)
{
  /*
   *   ┌─15                 ┌─15
   *  10     --erase 1-->  10
   *   └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 1 */ " 1  0 -1 -1\n"
      /* 2 */ "10  0  1  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 2 */ "10  1 -1  0\n"
  );

  int erase_value = 1;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_three_nodes_right)
{
  /*
   *   ┌─15
   *  10     --erase 1-->  10
   *   └─1                  └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 1 */ " 1  0 -1 -1\n"
      /* 2 */ "10  0  1  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ " 1  0 -1 -1\n"
      /* 2 */ "10 -1  0 -1\n"
  );

  int erase_value = 15;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_three_nodes_root)
{
  /*
   *   ┌─15
   *  10     --erase 10-->  15
   *   └─1                   └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "15  0 -1 -1\n"
      /* 1 */ " 1  0 -1 -1\n"
      /* 2 */ "10  0  1  0\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 2 */ " 1  0 -1 -1\n"
      /* 0 */ "15 -1  0 -1\n"
  );

  int erase_value = 10;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_case3)
{
  /*
   *          ┌─14                         ┌─14
   *       ┌─13                         ┌─13
   *    ┌─12                         ┌─12
   *    │  └─11                      │  └─11
   *  ┌─10                        ┌─10
   *  │ │ ┌─9                     │  │ ┌─9
   *  │ └─8         --erase 5-->  │  └─8
   *  │   │ ┌─7                   │    └─7
   *  │   └─6                     6
   *  5                           │ ┌─4
   *  │ ┌─4                       └─3
   *  └─3                           └─2
   *    └─2                           └─1
   *      └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /*  0 */ "14  0 -1 -1\n"
      /*  1 */ "13  1 -1  0\n"
      /*  2 */ "11  0 -1 -1\n"
      /*  3 */ "12  1  2  1\n"
      /*  4 */ " 9  0 -1 -1\n"
      /*  5 */ " 7  0 -1 -1\n"
      /*  6 */ " 6  1 -1  5\n"
      /*  7 */ " 8 -1  6  4\n"
      /*  8 */ "10  0  7  3\n"
      /*  9 */ " 4  0 -1 -1\n"
      /* 10 */ " 1  0 -1 -1\n"
      /* 11 */ " 2 -1 10 -1\n"
      /* 12 */ " 3 -1 11  9\n"
      /* 13 */ " 5  1 12  8\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /*  0 */ "14  0 -1 -1\n"
      /*  1 */ "13  1 -1  0\n"
      /*  2 */ "11  0 -1 -1\n"
      /*  3 */ "12  1  2  1\n"
      /*  4 */ " 9  0 -1 -1\n"
      /*  5 */ " 7  0 -1 -1\n"
      /*  6 */ " 8  0  5  4\n"
      /*  7 */ "10  0  6  3\n"
      /*  8 */ " 4  0 -1 -1\n"
      /*  9 */ " 1  0 -1 -1\n"
      /* 10 */ " 2 -1  9 -1\n"
      /* 11 */ " 3 -1 10  8\n"
      /* 12 */ " 6  1 11  7\n"
  );

  int erase_value = 5;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_left_double_rotation)
{
  /*
   *    ┌─7                    ┌─7
   *  ┌─6                    ┌─6
   *  5                      │ └─4
   *  │   ┌─4  --erase 5-->  3
   *  │ ┌─3                  └─2
   *  └─2                      └─1
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "6  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "3  1 -1  2\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2  1  4  3\n"
      /* 6 */ "5 -1  5  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "4  0 -1 -1\n"
      /* 2 */ "6  0  1  0\n"
      /* 3 */ "1  0 -1 -1\n"
      /* 4 */ "2 -1  3 -1\n"
      /* 5 */ "3  0  4  2\n"
  );

  int erase_value = 5;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_left_double_rotation_symmetric)
{
  /*
   *    ┌─7                    ┌─7
   *  ┌─6                    ┌─6
   *  │ └─5                  │ └─5
   *  │   └─4  --erase 3-->  4
   *  3                      └─2
   *  └─2                      └─1
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "4  0 -1 -1\n"
      /* 2 */ "5 -1  1 -1\n"
      /* 3 */ "6 -1  2  0\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2 -1  4 -1\n"
      /* 6 */ "3  1  5  3\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "5  0 -1 -1\n"
      /* 2 */ "6  0  1  0\n"
      /* 3 */ "1  0 -1 -1\n"
      /* 4 */ "2 -1  3 -1\n"
      /* 5 */ "4  0  4  2\n"
  );

  int erase_value = 3;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_right_double_rotation)
{
  /*
   *    ┌─7                    ┌─7
   *  ┌─6                    ┌─6
   *  │ └─5                  5
   *  │   └─4  --erase 1-->  │ ┌─4
   *  3                      └─3
   *  └─2                      └─2
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "4  0 -1 -1\n"
      /* 2 */ "5 -1  1 -1\n"
      /* 3 */ "6 -1  2  0\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2 -1  4 -1\n"
      /* 6 */ "3  1  5  3\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "6  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "2  0 -1 -1\n"
      /* 4 */ "3  0  3  2\n"
      /* 5 */ "5  0  4  1\n"
  );

  int erase_value = 1;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_right_double_rotation_symmetric)
{
  /*
   *    ┌─7                    ┌─6
   *  ┌─6                    ┌─5
   *  5                      │ └─4
   *  │   ┌─4  --erase 7-->  3
   *  │ ┌─3                  └─2
   *  └─2                      └─1
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "6  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "3  1 -1  2\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2  1  4  3\n"
      /* 6 */ "5 -1  5  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "6  0 -1 -1\n"
      /* 1 */ "4  0 -1 -1\n"
      /* 2 */ "5  0  1  0\n"
      /* 3 */ "1  0 -1 -1\n"
      /* 4 */ "2 -1  3 -1\n"
      /* 5 */ "3  0  4  2\n"
  );

  int erase_value = 7;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_left_single_rotation)
{
  /*
   *    ┌─7                    ┌─7
   *  ┌─6                    ┌─6
   *  5                      │ └─4
   *  │        --erase 5-->  3
   *  │ ┌─4                  └─2
   *  └─3                      └─1
   *    └─2
   *      └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "6  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "1  0 -1 -1\n"
      /* 4 */ "2 -1  3 -1\n"
      /* 5 */ "3 -1  4  2\n"
      /* 6 */ "5 -1  5  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "4  0 -1 -1\n"
      /* 2 */ "6  0  1  0\n"
      /* 3 */ "1  0 -1 -1\n"
      /* 4 */ "2 -1  3 -1\n"
      /* 5 */ "3  0  4  2\n"
  );

  int erase_value = 5;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_right_single_rotation)
{
  /*
   *      ┌─7
   *    ┌─6                    ┌─7
   *  ┌─5                    ┌─6
   *  │ └─4                  5
   *  3        --erase 1-->  │ ┌─4
   *  └─2                    └─3
   *    └─1                    └─2
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "6  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "5  1  2  1\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2 -1  4 -1\n"
      /* 6 */ "3  1  5  3\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "7  0 -1 -1\n"
      /* 1 */ "6  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "2  0 -1 -1\n"
      /* 4 */ "3  0  3  2\n"
      /* 5 */ "5  0  4  1\n"
  );

  int erase_value = 1;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_left_single_rotation_stop)
{
  /*
   *    ┌─8                    ┌─8
   *  ┌─7                    ┌─7
   *  6                      │ └─5
   *  │ ┌─5                  │   └─4
   *  │ │ └─4  --erase 6-->  3
   *  └─3                    └─2
   *    └─2                    └─1
   *      └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "7  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "5 -1  2 -1\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2 -1  4 -1\n"
      /* 6 */ "3  0  5  3\n"
      /* 7 */ "6 -1  6  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "4  0 -1 -1\n"
      /* 2 */ "5 -1  1 -1\n"
      /* 3 */ "7 -1  2  0\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2 -1  4 -1\n"
      /* 6 */ "3  1  5  3\n"
  );

  int erase_value = 6;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_right_single_rotation_stop)
{
  /*
   *      ┌─8                  ┌─8
   *    ┌─7                  ┌─7
   *  ┌─6                    6
   *  │ └─5    --erase 1-->  │ ┌─5
   *  │   └─4                │ │ └─4
   *  3                      └─3
   *  └─2                      └─2
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "7  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "5 -1  2 -1\n"
      /* 4 */ "6  0  3  1\n"
      /* 5 */ "1  0 -1 -1\n"
      /* 6 */ "2 -1  5 -1\n"
      /* 7 */ "3  1  6  4\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "7  1 -1  0\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "5 -1  2 -1\n"
      /* 4 */ "2  0 -1 -1\n"
      /* 5 */ "3 -1  4  3\n"
      /* 6 */ "6 -1  5  1\n"
  );

  int erase_value = 1;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_right_single_rotation_in_left_subtree)
{
  /*
   *    ┌─8                    ┌─8
   *  ┌─7                    ┌─7
   *  6                      6
   *  │   ┌─5  --erase 1-->  │ ┌─5
   *  │ ┌─4                  └─4
   *  │ │ └─3                  │ ┌─3
   *  └─2                      └─2
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "7  1 -1  0\n"
      /* 2 */ "5  0 -1 -1\n"
      /* 3 */ "3  0 -1 -1\n"
      /* 4 */ "4  0  3  2\n"
      /* 5 */ "1  0 -1 -1\n"
      /* 6 */ "2  1  5  4\n"
      /* 7 */ "6 -1  6  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "7  1 -1  0\n"
      /* 2 */ "5  0 -1 -1\n"
      /* 3 */ "3  0 -1 -1\n"
      /* 4 */ "2  1 -1  3\n"
      /* 5 */ "4 -1  4  2\n"
      /* 6 */ "6 -1  5  1\n"
  );

  int erase_value = 1;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_right_single_rotation_in_right_subtree)
{
  /*
   *      ┌─8                  ┌─8
   *    ┌─7                  ┌─7
   *    │ └─6                │ │ ┌─6
   *  ┌─5      --erase 4-->  │ └─5
   *  │ └─4                  3
   *  3                      └─2
   *  └─2                      └─1
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "6  0 -1 -1\n"
      /* 2 */ "7  0  1  0\n"
      /* 3 */ "4  0 -1 -1\n"
      /* 4 */ "5  1  3  2\n"
      /* 5 */ "1  0 -1 -1\n"
      /* 6 */ "2 -1  5 -1\n"
      /* 7 */ "3  1  6  4\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "6  0 -1 -1\n"
      /* 2 */ "5  1 -1  1\n"
      /* 3 */ "7 -1  2  0\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2 -1  4 -1\n"
      /* 6 */ "3  1  5  3\n"
  );

  int erase_value = 4;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_left_single_rotation_in_left_subtree)
{
  /*
   *    ┌─8                    ┌─8
   *  ┌─7                    ┌─7
   *  6                      6
   *  │ ┌─5    --erase 4-->  │ ┌─5
   *  └─4                    │ │ └─3
   *    │ ┌─3                └─2
   *    └─2                    └─1
   *      └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "7  1 -1  0\n"
      /* 2 */ "5  0 -1 -1\n"
      /* 3 */ "3  0 -1 -1\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2  0  4  3\n"
      /* 6 */ "4 -1  5  2\n"
      /* 7 */ "6 -1  6  1\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "7  1 -1  0\n"
      /* 2 */ "3  0 -1 -1\n"
      /* 3 */ "5 -1  2 -1\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2  1  4  3\n"
      /* 6 */ "6 -1  5  1\n"
  );

  int erase_value = 4;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree, &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root, node_cmp));
}


TEST_F(tree_fixture, avltree, erase_left_single_rotation_in_right_subtree)
{
  /*
   *    ┌─8                    ┌─8
   *  ┌─7                      │ └─6
   *  │ │ ┌─6                ┌─5
   *  │ └─5    --erase 7-->  │ └─4
   *  │   └─4                3
   *  3                      └─2
   *  └─2                      └─1
   *    └─1
   */

  fixture->in_nodes = ut_parse_avltree(&fixture->in_tree,
      /* 0 */ "8  0 -1 -1\n"
      /* 1 */ "6  0 -1 -1\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "5  0  2  1\n"
      /* 4 */ "7 -1  3  0\n"
      /* 5 */ "1  0 -1 -1\n"
      /* 6 */ "2 -1  5 -1\n"
      /* 7 */ "3  1  6  4\n"
  );

  fixture->expected_nodes = ut_parse_avltree(&fixture->expected_tree,
      /* 0 */ "6  0 -1 -1\n"
      /* 1 */ "8 -1  0 -1\n"
      /* 2 */ "4  0 -1 -1\n"
      /* 3 */ "5  1  2  1\n"
      /* 4 */ "1  0 -1 -1\n"
      /* 5 */ "2 -1  4 -1\n"
      /* 6 */ "3  1  5  3\n"
  );

  int erase_value = 7;
  dfk_avltree_hook_t* erase_node = dfk_avltree_find(&fixture->in_tree,
      &erase_value, node_int_find_cmp);
  dfk_avltree_erase(&fixture->in_tree, erase_node);
  EXPECT(ut_trees_equal(fixture->in_tree._root, fixture->expected_tree._root,
      node_cmp));
}


TEST_F(tree_fixture, avltree, size_empty)
{
  EXPECT(dfk_avltree_size(&fixture->in_tree) == 0);
}


TEST_F(tree_fixture, avltree, size_non_empty)
{
  size_t nnodes = 400;
  fixture->in_nodes = malloc(nnodes * sizeof(node_t));
  for (size_t i = 0; i < nnodes; ++i) {
    dfk_avltree_hook_init((dfk_avltree_hook_t*) (fixture->in_nodes + i));
    fixture->in_nodes[i].value = (757 * i + 839) % 967;
    dfk_avltree_insert(&fixture->in_tree,
        (dfk_avltree_hook_t*) (fixture->in_nodes + i));
  }
  EXPECT(dfk_avltree_size(&fixture->in_tree) == nnodes);
}

