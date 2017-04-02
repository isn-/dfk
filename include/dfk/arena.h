/**
 * @file dfk/internal/arena.h
 * An effective small object allocator
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <dfk/core.h>
#include <dfk/list.h>


typedef struct dfk_arena_t {
  dfk_t* dfk;

  /**
   * Data segments
   * @private
   */
  dfk_list_t _segments;

  /**
   * Objects with cleanup
   * @private
   */
  dfk_list_t _owc;

} dfk_arena_t;


int dfk_arena_init(dfk_arena_t* arena, dfk_t* dfk);
int dfk_arena_free(dfk_arena_t* arena);

void* dfk_arena_alloc(dfk_arena_t* arena, size_t size);
void* dfk_arena_alloc_copy(dfk_arena_t* arena, const char* data, size_t size);

typedef void (*dfk_arena_cleanup)(dfk_arena_t*, void*);

void* dfk_arena_alloc_ex(dfk_arena_t* arena, size_t size,
    dfk_arena_cleanup clean);

void* dfk_arena_alloc_ex_copy(dfk_arena_t* arena, const char* data, size_t size,
    dfk_arena_cleanup clean);

