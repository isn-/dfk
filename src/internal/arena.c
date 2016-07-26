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
#include <dfk/internal/arena.h>


typedef struct segment_t {
  dfk_list_hook_t hook;
  size_t size;
  size_t used;
} segment_t;


typedef struct owc_t {
  dfk_list_hook_t hook;
  dfk_arena_cleanup cleanup;
} owc_t;


int dfk_arena_init(dfk_arena_t* arena, dfk_t* dfk)
{
  assert(arena);
  assert(dfk);
  arena->dfk = dfk;
  dfk_list_init(&arena->_.segments);
  dfk_list_init(&arena->_.owc);
  return dfk_err_ok;
}


int dfk_arena_free(dfk_arena_t* arena)
{
  assert(arena);
  {
    dfk_list_hook_t* i = arena->_.owc.head;
    while (i) {
      ((owc_t*) i)->cleanup(arena, ((char*) i) + sizeof(owc_t));
      i = i->next;
    }
  }
  dfk_list_hook_t* i = arena->_.segments.head;
  while (i) {
    dfk_list_hook_t* next = i->next;
    DFK_FREE(arena->dfk, i);
    i = next;
  }
  return dfk_err_ok;
}


#define CURRENT_SEGMENT(arena) ((segment_t*) (arena)->_.segments.tail)


static size_t dfk__arena_bytes_available(dfk_arena_t* arena)
{
  assert(arena);
  {
    segment_t* seg = CURRENT_SEGMENT(arena);
    assert(seg);
    assert(seg->size > seg->used);
    return seg->size - seg->used;
  }
}


void* dfk_arena_alloc(dfk_arena_t* arena, size_t size)
{
  assert(arena);
  assert(size);

  if (!dfk_list_size(&arena->_.segments)
      || dfk__arena_bytes_available(arena) < size) {
    size_t toalloc = DFK_MAX(DFK_ARENA_SEGMENT_SIZE, size + sizeof(segment_t));
    segment_t* s = DFK_MALLOC(arena->dfk, toalloc);
    if (!s) {
      return NULL;
    }
    dfk_list_hook_init(&s->hook);
    dfk_list_append(&arena->_.segments, &s->hook);
    s->size = DFK_ARENA_SEGMENT_SIZE - sizeof(segment_t);
    s->used = sizeof(segment_t) + size;
    return ((char*) s) + sizeof(segment_t);
  }

  {
    segment_t* seg = CURRENT_SEGMENT(arena);
    void* ret = ((char*) seg) + seg->used;
    seg->used += size;
    return ret;
  }
}

void* dfk_arena_alloc_copy(dfk_arena_t* arena, const char* data, size_t size)
{
  assert(arena);
  assert(data);
  assert(size);
  void* allocated = dfk_arena_alloc(arena, size);
  if (allocated) {
    memcpy(allocated, data, size);
  }
  return allocated;
}


void* dfk_arena_alloc_ex(dfk_arena_t* arena, size_t size, dfk_arena_cleanup cleanup)
{
  assert(arena);
  assert(size);
  assert(cleanup);
  {
    owc_t* owc = dfk_arena_alloc(arena, sizeof(owc_t) + size);
    dfk_list_hook_init(&owc->hook);
    dfk_list_append(&arena->_.owc, &owc->hook);
    owc->cleanup = cleanup;
    return ((char*) owc) + sizeof(owc_t);
  }
}

