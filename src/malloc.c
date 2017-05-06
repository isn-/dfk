/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LISENSE)
 */

#include <assert.h>
#include <dfk/config.h>
#include <dfk/internal/malloc.h>

#define ALIGNED(ptr) (!(((ptrdiff_t) (ptr)) % DFK_STACK_ALIGNMENT))

void* dfk__malloc(dfk_t* dfk, size_t nbytes)
{
  assert(dfk);
  assert(nbytes);
  void* res = dfk->malloc(dfk, nbytes);
  assert(ALIGNED(res));
  return res;
}

void dfk__free(dfk_t* dfk, void* p)
{
  assert(dfk);
  assert(p);
  assert(ALIGNED(p));
  dfk->free(dfk, p);
}

/*
void* dfk__realloc(dfk_t* dfk, void* p, size_t nbytes)
{
  assert(dfk);
  assert(p);
  assert(nbytes);
  assert(ALIGNED(p));
  void* res = dfk->realloc(dfk, p, nbytes);
  assert(ALIGNED(res));
  return res;
}
*/
