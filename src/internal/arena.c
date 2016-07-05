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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

