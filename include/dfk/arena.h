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
#include <dfk/context.h>
#include <dfk/list.h>

/**
 * Memory arena, manages small object allocation.
 */
typedef struct dfk_arena_t {
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

/**
 * Initialize empty arena.
 */
void dfk_arena_init(dfk_arena_t* arena);

/**
 * Release all memory managed by arena.
 */
void dfk_arena_free(dfk_arena_t* arena, dfk_t* dfk);

/**
 * Allocate uninitialized buffer of the given size.
 */
void* dfk_arena_alloc(dfk_arena_t* arena, dfk_t* dfk, size_t size);

/**
 * Allocate and initialize buffer.
 */
void* dfk_arena_alloc_copy(dfk_arena_t* arena, dfk_t* dfk,
    const char* data, size_t size);

/**
 * A function to cleanup an object allocated within arena.
 */
typedef void (*dfk_arena_cleanup)(dfk_arena_t*, void*);

/**
 * Allocate uninitialized buffer of the given size, associate cleanup callback
 * with the buffer.
 */
void* dfk_arena_alloc_ex(dfk_arena_t* arena, dfk_t* dfk, size_t size,
    dfk_arena_cleanup clean);

/**
 * Allocate and Initialize buffer, associate cleanup callback with the buffer.
 */
void* dfk_arena_alloc_copy_ex(dfk_arena_t* arena, dfk_t* dfk,
    const char* data, size_t size, dfk_arena_cleanup clean);

