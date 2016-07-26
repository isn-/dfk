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

#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/arena.h>
#include <ut.h>


typedef struct fixture_t {
  dfk_t dfk;
  dfk_arena_t arena;
} fixture_t;


static void fixture_setup(fixture_t* f)
{
  dfk_init(&f->dfk);
  dfk_arena_init(&f->arena, &f->dfk);
}


static void fixture_teardown(fixture_t* f)
{
  dfk_arena_free(&f->arena);
  dfk_free(&f->dfk);
}


static int intersect(char* p1, char* r1, char* p2, char* r2)
{
  if (r1 > p1) {
    DFK_SWAP(char*, r1, p1);
  }
  if (r2 > p2) {
    DFK_SWAP(char*, r2, p2);
  }
  return ((p1 <= p2) && (p2 <= r1)) || ((p1 <= r2) && (r2 <= r1));
}


TEST_F(fixture, arena, alloc_first)
{
  size_t i, count = 10;
  char* p = dfk_arena_alloc(&fixture->arena, count);
  EXPECT(p);
  for (i = 0; i < count; ++i) {
    p[i] = 0;
  }
}


TEST_F(fixture, arena, alloc_no_overlap)
{
  size_t count = 10;
  char* p0 = dfk_arena_alloc(&fixture->arena, count);
  char* p1 = dfk_arena_alloc(&fixture->arena, count);
  char* p2 = dfk_arena_alloc(&fixture->arena, count);
  EXPECT(!intersect(p0, p0 + count, p1, p1 + count));
  EXPECT(!intersect(p1, p1 + count, p2, p2 + count));
  EXPECT(!intersect(p0, p0 + count, p2, p2 + count));
}


TEST_F(fixture, arena, alloc_two_segments)
{
  size_t i, count = (size_t) (0.75 * DFK_ARENA_SEGMENT_SIZE);
  signed char* p1 = dfk_arena_alloc(&fixture->arena, count);
  signed char* p2 = dfk_arena_alloc(&fixture->arena, count);
  for (i = 0; i < count; ++i) {
    p1[i] = (signed char) (2 * i);
    p2[i] = (signed char) (2 * i + 1);
  }
  for (i = 0; i < count; ++i) {
    EXPECT(p1[i] == (signed char) (2 * i));
    EXPECT(p2[i] == (signed char) (2 * i + 1));
  }
}


static void* out_of_memory(dfk_t* p, size_t size)
{
  DFK_UNUSED(p);
  DFK_UNUSED(size);
  return NULL;
}


TEST_F(fixture, arena, alloc_no_mem)
{
  fixture->dfk.malloc = out_of_memory;
  EXPECT(!dfk_arena_alloc(&fixture->arena, 10));
}


typedef struct data_holder_t {
  dfk_t* dfk;
  void* p;
} data_holder_t;


static void data_holder_init(dfk_t* dfk, data_holder_t* dh)
{
  dh->dfk = dfk;
  dh->p = DFK_MALLOC(dfk, 1024);
}


static void data_holder_free(data_holder_t* dh)
{
  DFK_FREE(dh->dfk, dh->p);
}


static void cleanup_data_holder(dfk_arena_t* arena, void* p)
{
  DFK_UNUSED(arena);
  data_holder_free((data_holder_t*) p);
}


TEST_F(fixture, arena, alloc_ex_one)
{
  data_holder_t* dh = dfk_arena_alloc_ex(&fixture->arena,
      sizeof(void*), cleanup_data_holder);
  EXPECT(dh);
  data_holder_init(&fixture->dfk, dh);
  /* Valgrind should report no memory leak here */
}


TEST_F(fixture, arena, alloc_mix)
{
  size_t count = (size_t) (0.4 * DFK_ARENA_SEGMENT_SIZE);
  size_t i = 0;
  for (i = 0; i < 10; ++i) {
    data_holder_t* dh = dfk_arena_alloc_ex(&fixture->arena,
        sizeof(void*), cleanup_data_holder);
    EXPECT(dh);
    data_holder_init(&fixture->dfk, dh);
    EXPECT(dfk_arena_alloc(&fixture->arena, count));
  }
  /* Valgrind should report no memory leak here */
}


TEST_F(fixture, arena, alloc_copy)
{
  char buf[] = "Hello, world";
  void* p = dfk_arena_alloc_copy(&fixture->arena, buf, sizeof(buf));
  EXPECT(p);
  EXPECT(memcmp(p, buf, sizeof(buf)) == 0);
}


TEST_F(fixture, arena, alloc_copy_no_mem)
{
  char buf[] = "Hello, world";
  fixture->dfk.malloc = out_of_memory;
  void* p = dfk_arena_alloc_copy(&fixture->arena, buf, sizeof(buf));
  EXPECT(!p);
}

