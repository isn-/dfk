/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <stdlib.h>
#include <dfk/internal.h>
#include <allocators.h>

void* malloc_first_n_ok(dfk_t* dfk, size_t size)
{
  size_t* nalloc = dfk->user.data;
  if (*nalloc) {
    (*nalloc)--;
    return malloc(size);
  }
  return NULL;
}

static void* aligned(void* p, size_t alignment)
{
  if ((ptrdiff_t) p % alignment) {
    return (char*) p + alignment - ((ptrdiff_t) p % alignment);
  }
  return p;
}

void* malloc_aligned(dfk_t* dfk, size_t size)
{
  alignment_t al = *((alignment_t*) dfk->user.data);
  assert(al.blocksize> sizeof(void*));
  char* rawptr = malloc(size + 2 * al.blocksize);
  char* rawptr_aligned = aligned(rawptr, al.blocksize);
  *((void**) rawptr_aligned) = rawptr;
  char* res = (char*) aligned(rawptr_aligned + 1, al.blocksize) + al.offset;
  return res;
}

void free_aligned(dfk_t* dfk, void* p)
{
  alignment_t al = *((alignment_t*) dfk->user.data);
  assert(al.blocksize> sizeof(void*));
  void* rawptr = *((void**) (char*) p - al.offset - al.blocksize);
  free(rawptr);
}

void* realloc_aligned(dfk_t* dfk, void* p, size_t size)
{
  DFK_UNUSED(dfk);
  DFK_UNUSED(size);
  return p;
}

