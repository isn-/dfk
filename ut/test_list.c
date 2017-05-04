/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */


#include <dfk/list.h>
#include <dfk/internal.h>
#include <ut.h>


typedef struct {
  dfk_list_hook_t hook;
  int value;
} myint_t;


static void myint_init(myint_t* i, int value)
{
  i->value = value;
  dfk_list_hook_init(&i->hook);
}


typedef struct {
  dfk_list_t l;
  myint_t values[3];
} myint_fixture_t;


static void myint_fixture_setup(myint_fixture_t* f)
{
  size_t i;
  for (i = 0; i < DFK_SIZE(f->values); ++i) {
    myint_init(f->values + i, 10 + i);
  }
  dfk_list_init(&f->l);
}


static void myint_fixture_teardown(myint_fixture_t* f)
{
  DFK_UNUSED(f);
}


TEST(list, sizeof)
{
  EXPECT(dfk_list_sizeof() == sizeof(dfk_list_t));
}


TEST(list, iterator_sizeof)
{
  EXPECT(dfk_list_it_sizeof() == sizeof(dfk_list_it));
}


TEST(list, reverse_iterator_sizeof)
{
  EXPECT(dfk_list_rit_sizeof() == sizeof(dfk_list_rit));
}


TEST(list, init_free)
{
  dfk_list_t l;
  dfk_list_init(&l);
  EXPECT(dfk_list_size(&l) == 0);
}


TEST_F(myint_fixture, list, append)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_append(l, &fixture->values[1].hook);
  EXPECT(dfk_list_size(l) == 1);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t* ) it.value)->value == 11);

  dfk_list_append(l, &fixture->values[0].hook);
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 11);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 10);

  dfk_list_append(&fixture->l, &fixture->values[2].hook);
  EXPECT(dfk_list_size(l) == 3);
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 11);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 12);
}


TEST_F(myint_fixture, list, prepend)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_prepend(l, &fixture->values[1].hook);
  EXPECT(dfk_list_size(l) == 1);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 11);

  dfk_list_prepend(&fixture->l, &fixture->values[0].hook);
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 11);

  dfk_list_prepend(&fixture->l, &fixture->values[2].hook);
  EXPECT(dfk_list_size(l) == 3);
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 12);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 11);
}


TEST(list, erase_last)
{
  dfk_list_t l;
  myint_t v;

  myint_init(&v, 10);
  dfk_list_init(&l);
  dfk_list_append(&l, &v.hook);
  dfk_list_rit it;
  dfk_list_rbegin(&l, &it);
  dfk_list_rerase(&l, &it);
  EXPECT(dfk_list_size(&l) == 0);
}


TEST_F(myint_fixture, list, erase_from_head)
{
  size_t i;
  dfk_list_t* l = &fixture->l;
  for (i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_it it;
  dfk_list_begin(l, &it);
  dfk_list_erase(l, &it);
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 11);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 12);
}


TEST_F(myint_fixture, list, erase_from_tail)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  {
    dfk_list_rit it;
    dfk_list_rbegin(l, &it);
    dfk_list_rerase(l, &it);
    EXPECT(dfk_list_size(l) == 2);
  }
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 11);
}


TEST_F(myint_fixture, list, erase_from_mid)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_it it;
  dfk_list_begin(l, &it);
  dfk_list_it_next(&it);
  dfk_list_erase(l, &it);
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 12);
}


TEST_F(myint_fixture, list, clear_non_empty)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_clear(l);
  EXPECT(dfk_list_size(l) == 0);
}


TEST_F(myint_fixture, list, clear_empty)
{
  dfk_list_clear(&fixture->l);
  EXPECT(dfk_list_size(&fixture->l) == 0);
}


TEST_F(myint_fixture, list, pop_back)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_pop_back(l);
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 11);
}


TEST_F(myint_fixture, list, pop_front)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_pop_front(l);
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 11);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 12);
}


TEST_F(myint_fixture, list, insert_empty)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_it end;
  dfk_list_end(l, &end);
  dfk_list_insert(l, &fixture->values[0].hook, &end);
  EXPECT(dfk_list_size(l) == 1);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
}


TEST_F(myint_fixture, list, insert_1_begin)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_append(l, &fixture->values[0].hook);
  {
    dfk_list_it it;
    dfk_list_begin(l, &it);
    dfk_list_insert(l, &fixture->values[1].hook, &it);
  }
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 11);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 10);
}


TEST_F(myint_fixture, list, insert_1_end)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_append(l, &fixture->values[0].hook);
  {
    dfk_list_it it;
    dfk_list_end(l, &it);
    dfk_list_insert(l, &fixture->values[1].hook, &it);
  }
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 11);
}


TEST_F(myint_fixture, list, insert_begin)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  myint_t new;
  myint_init(&new, 6);
  {
    dfk_list_it it;
    dfk_list_begin(l, &it);
    dfk_list_insert(l, &new.hook, &it);
  }
  EXPECT(dfk_list_size(l) == 4);
  int expected_values[] = {6, 10, 11, 12};
  dfk_list_it it, end;
  dfk_list_begin(l, &it);
  dfk_list_end(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) it.value)->value == expected_values[i]);
    dfk_list_it_next(&it);
  }
  EXPECT(dfk_list_it_equal(&it, &end));
}


TEST_F(myint_fixture, list, insert_end)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  myint_t new;
  myint_init(&new, 6);
  {
    dfk_list_it it;
    dfk_list_end(l, &it);
    dfk_list_insert(l, &new.hook, &it);
  }
  EXPECT(dfk_list_size(l) == 4);
  int expected_values[] = {10, 11, 12, 6};
  dfk_list_it it, end;
  dfk_list_begin(l, &it);
  dfk_list_end(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) it.value)->value == expected_values[i]);
    dfk_list_it_next(&it);
  }
  EXPECT(dfk_list_it_equal(&it, &end));
}


TEST_F(myint_fixture, list, insert_mid)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  myint_t new;
  myint_init(&new, 6);
  {
    dfk_list_it it;
    dfk_list_begin(l, &it);
    dfk_list_it_next(&it);
    dfk_list_insert(l, &new.hook, &it);
  }
  EXPECT(dfk_list_size(l) == 4);
  int expected_values[] = {10, 6, 11, 12};
  dfk_list_it it, end;
  dfk_list_begin(l, &it);
  dfk_list_end(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) it.value)->value == expected_values[i]);
    dfk_list_it_next(&it);
  }
  EXPECT(dfk_list_it_equal(&it, &end));
}


TEST_F(myint_fixture, list, rinsert_empty)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_rit end;
  dfk_list_rend(l, &end);
  dfk_list_rinsert(l, &fixture->values[0].hook, &end);
  EXPECT(dfk_list_size(l) == 1);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
}


TEST_F(myint_fixture, list, rinsert_1_begin)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_append(l, &fixture->values[0].hook);
  {
    dfk_list_rit rit;
    dfk_list_rbegin(l, &rit);
    dfk_list_rinsert(l, &fixture->values[1].hook, &rit);
  }
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 10);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 11);
}


TEST_F(myint_fixture, list, rinsert_1_end)
{
  dfk_list_t* l = &fixture->l;
  dfk_list_append(l, &fixture->values[0].hook);
  {
    dfk_list_rit rit;
    dfk_list_rend(l, &rit);
    dfk_list_rinsert(l, &fixture->values[1].hook, &rit);
  }
  EXPECT(dfk_list_size(l) == 2);
  dfk_list_it it;
  dfk_list_begin(l, &it);
  EXPECT(((myint_t*) it.value)->value == 11);
  dfk_list_it_next(&it);
  EXPECT(((myint_t*) it.value)->value == 10);
}


TEST_F(myint_fixture, list, rinsert_begin)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  myint_t new;
  myint_init(&new, 6);
  {
    dfk_list_rit rit;
    dfk_list_rbegin(l, &rit);
    dfk_list_rinsert(l, &new.hook, &rit);
  }
  EXPECT(dfk_list_size(l) == 4);
  int expected_values[] = {10, 11, 12, 6};
  dfk_list_it it, end;
  dfk_list_begin(l, &it);
  dfk_list_end(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) it.value)->value == expected_values[i]);
    dfk_list_it_next(&it);
  }
  EXPECT(dfk_list_it_equal(&it, &end));
}


TEST_F(myint_fixture, list, rinsert_end)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  myint_t new;
  myint_init(&new, 6);
  {
    dfk_list_rit rit;
    dfk_list_rend(l, &rit);
    dfk_list_rinsert(l, &new.hook, &rit);
  }
  EXPECT(dfk_list_size(l) == 4);
  int expected_values[] = {6, 10, 11, 12};
  dfk_list_it it, end;
  dfk_list_begin(l, &it);
  dfk_list_end(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) it.value)->value == expected_values[i]);
    dfk_list_it_next(&it);
  }
  EXPECT(dfk_list_it_equal(&it, &end));
}


TEST_F(myint_fixture, list, rinsert_mid)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  myint_t new;
  myint_init(&new, 6);
  {
    dfk_list_rit rit;
    dfk_list_rbegin(l, &rit);
    dfk_list_rit_next(&rit);
    dfk_list_rinsert(l, &new.hook, &rit);
  }
  EXPECT(dfk_list_size(l) == 4);
  int expected_values[] = {10, 11, 6, 12};
  dfk_list_it it, end;
  dfk_list_begin(l, &it);
  dfk_list_end(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) it.value)->value == expected_values[i]);
    dfk_list_it_next(&it);
  }
  EXPECT(dfk_list_it_equal(&it, &end));
}


TEST_F(myint_fixture, list, empty)
{
  dfk_list_t* l = &fixture->l;
  EXPECT(dfk_list_empty(l));
  dfk_list_prepend(l, &fixture->values[2].hook);
  EXPECT(!dfk_list_empty(l));
}


TEST_F(myint_fixture, list, iteration_forward)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  int expected_values[] = {10, 11, 12};
  dfk_list_it it, end;
  dfk_list_begin(l, &it);
  dfk_list_end(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) it.value)->value == expected_values[i]);
    dfk_list_it_next(&it);
  }
  EXPECT(dfk_list_it_equal(&it, &end));
}


TEST_F(myint_fixture, list, iteration_backward)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_prepend(l, &fixture->values[i].hook);
  }
  int expected_values[] = {10, 11, 12};
  dfk_list_rit rit, end;
  dfk_list_rbegin(l, &rit);
  dfk_list_rend(l, &end);
  for (size_t i = 0; i < DFK_SIZE(expected_values); ++i) {
    EXPECT(((myint_t*) rit.value)->value == expected_values[i]);
    dfk_list_rit_next(&rit);
  }
  EXPECT(dfk_list_rit_equal(&rit, &end));
}

TEST_F(myint_fixture, list, back)
{
  dfk_list_t* l = &fixture->l;
  EXPECT(dfk_list_back(l) == NULL);
  dfk_list_append(l, &fixture->values[0].hook);
  EXPECT(dfk_list_back(l));
  EXPECT(((myint_t*) dfk_list_back(l))->value == 10);
  dfk_list_append(l, &fixture->values[1].hook);
  EXPECT(((myint_t*) dfk_list_back(l))->value == 11);
}

TEST_F(myint_fixture, list, front)
{
  dfk_list_t* l = &fixture->l;
  EXPECT(dfk_list_front(l) == NULL);
  dfk_list_prepend(l, &fixture->values[0].hook);
  EXPECT(dfk_list_front(l));
  EXPECT(((myint_t*) dfk_list_front(l))->value == 10);
  dfk_list_prepend(l, &fixture->values[1].hook);
  EXPECT(((myint_t*) dfk_list_front(l))->value == 11);
}

TEST(list, move_empty_empty)
{
  dfk_list_t l1, l2;
  dfk_list_init(&l1);
  dfk_list_init(&l2);
  dfk_list_move(&l1, &l2);
  EXPECT(dfk_list_empty(&l1));
  EXPECT(dfk_list_empty(&l2));
}

TEST_F(myint_fixture, list, move_full_empty)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_t dst;
  dfk_list_init(&dst);
  dfk_list_move(l, &dst);
  EXPECT(dfk_list_empty(l));
  EXPECT(dfk_list_size(&dst) == DFK_SIZE(fixture->values));
  dfk_list_it it, end;
  dfk_list_begin(&dst, &it);
  dfk_list_end(&dst, &end);
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    EXPECT(((myint_t*) it.value)->value == (int) (10 + i));
    dfk_list_it_next(&it);
  }
}

TEST_F(myint_fixture, list, move_empty_full)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_t src;
  dfk_list_init(&src);
  dfk_list_move(&src, l);
  EXPECT(dfk_list_empty(l));
  EXPECT(dfk_list_empty(&src));
}

TEST_F(myint_fixture, list, move_full_full)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_t dst;
  dfk_list_init(&dst);
  myint_t dstvalues[10];
  for (size_t i = 0; i < DFK_SIZE(dstvalues); ++i) {
    myint_init(dstvalues + i, i + 20);
    dfk_list_append(&dst, (dfk_list_hook_t*) (dstvalues + i));
  }
  dfk_list_move(l, &dst);
  EXPECT(dfk_list_empty(l));
  EXPECT(dfk_list_size(&dst) == DFK_SIZE(fixture->values));
  dfk_list_it it, end;
  dfk_list_begin(&dst, &it);
  dfk_list_end(&dst, &end);
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    EXPECT(((myint_t*) it.value)->value == (int) (10 + i));
    dfk_list_it_next(&it);
  }
}

TEST(list, swap_empty_empty)
{
  dfk_list_t l1, l2;
  dfk_list_init(&l1);
  dfk_list_init(&l2);
  dfk_list_swap(&l1, &l2);
  EXPECT(dfk_list_empty(&l1));
  EXPECT(dfk_list_empty(&l2));
}

TEST_F(myint_fixture, list, swap_empty_full)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_t dst;
  dfk_list_init(&dst);
  dfk_list_swap(&dst, l);
  EXPECT(dfk_list_empty(l));
  EXPECT(dfk_list_size(&dst) == DFK_SIZE(fixture->values));
  dfk_list_it it, end;
  dfk_list_begin(&dst, &it);
  dfk_list_end(&dst, &end);
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    EXPECT(((myint_t*) it.value)->value == (int) (10 + i));
    dfk_list_it_next(&it);
  }
}

TEST_F(myint_fixture, list, swap_full_empty)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_t dst;
  dfk_list_init(&dst);
  dfk_list_swap(l, &dst);
  EXPECT(dfk_list_empty(l));
  EXPECT(dfk_list_size(&dst) == DFK_SIZE(fixture->values));
  dfk_list_it it, end;
  dfk_list_begin(&dst, &it);
  dfk_list_end(&dst, &end);
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    EXPECT(((myint_t*) it.value)->value == (int) (10 + i));
    dfk_list_it_next(&it);
  }
}

TEST_F(myint_fixture, list, swap_full_full)
{
  dfk_list_t* l = &fixture->l;
  for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(l, &fixture->values[i].hook);
  }
  dfk_list_t dst;
  dfk_list_init(&dst);
  myint_t dstvalues[10];
  for (size_t i = 0; i < DFK_SIZE(dstvalues); ++i) {
    myint_init(dstvalues + i, i + 20);
    dfk_list_append(&dst, (dfk_list_hook_t*) (dstvalues + i));
  }
  dfk_list_swap(l, &dst);
  {
    EXPECT(dfk_list_size(l) == DFK_SIZE(dstvalues));
    dfk_list_it it, end;
    dfk_list_begin(l, &it);
    dfk_list_end(l, &end);
    for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
      EXPECT(((myint_t*) it.value)->value == (int) (20 + i));
      dfk_list_it_next(&it);
    }
  }
  {
    EXPECT(dfk_list_size(&dst) == DFK_SIZE(fixture->values));
    dfk_list_it it, end;
    dfk_list_begin(&dst, &it);
    dfk_list_end(&dst, &end);
    for (size_t i = 0; i < DFK_SIZE(fixture->values); ++i) {
      EXPECT(((myint_t*) it.value)->value == (int) (10 + i));
      dfk_list_it_next(&it);
    }
  }
}

