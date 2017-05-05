/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/context.h>

/**
 * Malloc will always fail after the first N successfull allocations.
 *
 * Number of allocations should be specified in the dfk.user.data field, as a
 * pointer to size_t.
 */
void* malloc_first_n_ok(dfk_t* dfk, size_t size);

typedef struct alignment_t {
  size_t offset;
  size_t blocksize;
} alignment_t;

/**
 * Malloc returns a pointer aligned to N bytes
 *
 * Alignment should be specified in the dfk.user.data field, as a pointer to
 * alignment_t structure.
 *
 * @note One have to set free_aligned() and realloc_aligned() as free and
 * realloc functions as well.
 *
 * @see free_aligned, realloc_aligned
 */
void* malloc_aligned(dfk_t* dfk, size_t size);

/**
 * A corresponding `free' to malloc_aligned()
 */
void free_aligned(dfk_t* dfk, void* p);

/**
 * A corresponding `realloc' to malloc_aligned()
 */
void* realloc_aligned(dfk_t* dfk, void* p, size_t size);

