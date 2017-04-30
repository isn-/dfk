/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <dfk/arena.h>
#include <dfk/internal.h>

typedef struct segment_t {
  dfk_list_hook_t hook;
  size_t size;
  size_t used;
} segment_t;

/**
 * A structure to hold Object With Cleanup (OWC)
 */
typedef struct owc_t {
  dfk_list_hook_t hook;
  dfk_arena_cleanup cleanup;
} owc_t;

void dfk_arena_init(dfk_arena_t* arena)
{
  assert(arena);
  dfk_list_init(&arena->_segments);
  dfk_list_init(&arena->_owc);
}

void dfk_arena_free(dfk_arena_t* arena, dfk_t* dfk)
{
  assert(arena);
  assert(dfk);

  /* release objects with cleanup */
  dfk_list_it it, end;
  dfk_list_begin(&arena->_owc, &it);
  dfk_list_end(&arena->_owc, &end);
  while (!dfk_list_it_equal(&it, &end)) {
    ((owc_t*) it.value)->cleanup(arena, ((char*) it.value) + sizeof(owc_t));
    dfk_list_it_next(&it);
  }

  /* release plain segments */
  while (!dfk_list_empty(&arena->_segments)) {
    dfk_list_it begin;
    dfk_list_begin(&arena->_segments, &begin);
    dfk_list_hook_t* segment = begin.value;
    dfk_list_pop_front(&arena->_segments);
    DFK_FREE(dfk, segment);
  }
}

static segment_t* dfk__arena_current_segment(dfk_arena_t* arena)
{
  dfk_list_rit rbegin;
  dfk_list_rbegin(&arena->_segments, &rbegin);
  return (segment_t*) rbegin.value;
}

static size_t dfk__arena_bytes_available(dfk_arena_t* arena)
{
  assert(arena);
  segment_t* seg = dfk__arena_current_segment(arena);
  assert(seg);
  assert(seg->size > seg->used);
  return seg->size - seg->used;
}

void* dfk_arena_alloc(dfk_arena_t* arena, dfk_t* dfk, size_t size)
{
  assert(arena);
  assert(dfk);
  assert(size);

  if (dfk_list_empty(&arena->_segments)
      || dfk__arena_bytes_available(arena) < size) {
    size_t toalloc = DFK_MAX(DFK_ARENA_SEGMENT_SIZE, size + sizeof(segment_t));
    segment_t* s = DFK_MALLOC(dfk, toalloc);
    if (!s) {
      return NULL;
    }
    dfk_list_hook_init(&s->hook);
    dfk_list_append(&arena->_segments, &s->hook);
    s->size = DFK_ARENA_SEGMENT_SIZE - sizeof(segment_t);
    s->used = sizeof(segment_t) + size;
    return ((char*) s) + sizeof(segment_t);
  }

  segment_t* seg = dfk__arena_current_segment(arena);
  void* ret = ((char*) seg) + seg->used;
  seg->used += size;
  return ret;
}

void* dfk_arena_alloc_copy(dfk_arena_t* arena, dfk_t* dfk,
    const char* data, size_t size)
{
  assert(arena);
  assert(dfk);
  assert(data);
  assert(size);

  void* allocated = dfk_arena_alloc(arena, dfk, size);
  if (allocated) {
    memcpy(allocated, data, size);
  }
  return allocated;
}

void* dfk_arena_alloc_ex(dfk_arena_t* arena, dfk_t* dfk, size_t size,
    dfk_arena_cleanup cleanup)
{
  assert(arena);
  assert(size);
  assert(cleanup);
  owc_t* owc = dfk_arena_alloc(arena, dfk, sizeof(owc_t) + size);
  if (!owc) {
    return NULL;
  }
  dfk_list_hook_init(&owc->hook);
  dfk_list_append(&arena->_owc, &owc->hook);
  owc->cleanup = cleanup;
  return ((char*) owc) + sizeof(owc_t);
}

void* dfk_arena_alloc_copy_ex(dfk_arena_t* arena, dfk_t* dfk,
    const char* data, size_t size, dfk_arena_cleanup cleanup)
{
  assert(arena);
  assert(dfk);
  assert(data);
  assert(size);
  assert(cleanup);
  void* allocated  = dfk_arena_alloc_ex(arena, dfk, size, cleanup);
  if (allocated) {
    memcpy(allocated, data, size);
  }
  return allocated;
}

