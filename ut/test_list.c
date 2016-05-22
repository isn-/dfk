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
#include <dfk/internal/list.h>
#include "ut.h"


typedef struct {
  dfk_list_hook_t hook;
  int value;
} myint_t;


static void myint_init(myint_t* i, int value)
{
  i->value = value;
  dfk_list_hook_init(&i->hook);
}


static void myint_free(myint_t* i)
{
  dfk_list_hook_free(&i->hook);
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
  size_t i;
  dfk_list_free(&f->l);
  for (i = 0; i < DFK_SIZE(f->values); ++i) {
    myint_free(f->values + i);
  }
}


TEST(list, init_free)
{
  dfk_list_t l;
  dfk_list_init(&l);
  EXPECT(dfk_list_size(&l) == 0);
  dfk_list_free(&l);
}


TEST_F(myint_fixture, list, append)
{
  dfk_list_append(&fixture->l, &fixture->values[1].hook);
  EXPECT(fixture->l.head == fixture->l.tail);
  EXPECT(fixture->l.size == 1);
  EXPECT(((myint_t* ) fixture->l.head)->value == 11);

  dfk_list_append(&fixture->l, &fixture->values[0].hook);
  EXPECT(fixture->l.head->next = fixture->l.tail);
  EXPECT(fixture->l.tail->prev = fixture->l.head);
  EXPECT(fixture->l.size == 2);
  EXPECT(((myint_t*) fixture->l.head)->value == 11);
  EXPECT(((myint_t*) fixture->l.tail)->value == 10);

  dfk_list_append(&fixture->l, &fixture->values[2].hook);
  EXPECT(fixture->l.head->prev == NULL);
  EXPECT(fixture->l.tail->next == NULL);
  EXPECT(fixture->l.size == 3);
  EXPECT(((myint_t*) fixture->l.head)->value == 11);
  {
    myint_t* mid = (myint_t*) fixture->l.head->next;
    EXPECT(mid->value == 10);
  }
  EXPECT(((myint_t*) fixture->l.tail)->value == 12);
}


TEST_F(myint_fixture, list, prepend)
{
  dfk_list_prepend(&fixture->l, &fixture->values[1].hook);
  EXPECT(fixture->l.head == fixture->l.tail);
  EXPECT(fixture->l.size == 1);
  EXPECT(((myint_t*) fixture->l.head)->value == 11);

  dfk_list_prepend(&fixture->l, &fixture->values[0].hook);
  EXPECT(fixture->l.head->next = fixture->l.tail);
  EXPECT(fixture->l.tail->prev = fixture->l.head);
  EXPECT(fixture->l.size == 2);
  EXPECT(((myint_t*) fixture->l.head)->value == 10);
  EXPECT(((myint_t*) fixture->l.tail)->value == 11);

  dfk_list_prepend(&fixture->l, &fixture->values[2].hook);
  EXPECT(fixture->l.head->prev == NULL);
  EXPECT(fixture->l.tail->next == NULL);
  EXPECT(fixture->l.size == 3);
  EXPECT(((myint_t*) fixture->l.head)->value == 12);
  {
    myint_t* mid = (myint_t*) fixture->l.head->next;
    EXPECT(mid->value == 10);
  }
  EXPECT(((myint_t*) fixture->l.tail)->value == 11);
}


TEST(list, erase_last)
{
  dfk_list_t l;
  myint_t v;

  myint_init(&v, 10);
  dfk_list_init(&l);
  dfk_list_append(&l, &v.hook);
  dfk_list_erase(&l, &v.hook);
  EXPECT(dfk_list_size(&l) == 0);
}


TEST_F(myint_fixture, list, erase_from_head)
{
  size_t i;
  for (i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(&fixture->l, &fixture->values[i].hook);
  }
  dfk_list_erase(&fixture->l, fixture->l.head);
  EXPECT(dfk_list_size(&fixture->l) == 2);
  EXPECT(((myint_t*) fixture->l.head)->value == 11);
  EXPECT(((myint_t*) fixture->l.tail)->value == 12);
}


TEST_F(myint_fixture, list, erase_from_tail)
{
  size_t i;
  for (i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(&fixture->l, &fixture->values[i].hook);
  }
  dfk_list_erase(&fixture->l, fixture->l.tail);
  EXPECT(dfk_list_size(&fixture->l) == 2);
  EXPECT(((myint_t*) fixture->l.head)->value == 10);
  EXPECT(((myint_t*) fixture->l.tail)->value == 11);
}


TEST_F(myint_fixture, list, erase_from_mid)
{
  size_t i;
  for (i = 0; i < DFK_SIZE(fixture->values); ++i) {
    dfk_list_append(&fixture->l, &fixture->values[i].hook);
  }
  dfk_list_erase(&fixture->l, fixture->l.head->next);
  EXPECT(dfk_list_size(&fixture->l) == 2);
  EXPECT(((myint_t*) fixture->l.head)->value == 10);
  EXPECT(((myint_t*) fixture->l.tail)->value == 12);
}

