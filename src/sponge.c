/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <string.h>
#include <dfk/config.h>
#include <dfk/sponge.h>
#include <dfk/malloc.h>
#include <dfk/error.h>
#include <dfk/internal.h>

#define DFK_SPONGE_INITIAL_SIZE DFK_PAGE_SIZE

#ifdef NDEBUG
#define DFK_SPONGE_CHECK_INVARIANTS(sponge) DFK_UNUSED(sponge)
#else

static void dfk__sponge_check_invariants(dfk__sponge_t* sponge)
{
  assert(sponge);
  assert(sponge->dfk);
  if (!sponge->base) {
    assert(!sponge->size);
    assert(!sponge->cur);
    assert(!sponge->capacity);
  } else {
    assert(sponge->cur);
    assert(sponge->base <= sponge->cur);
    assert(sponge->cur <= sponge->base + sponge->size);
    assert(sponge->base + sponge->size <= sponge->base + sponge->capacity);
  }
}

#define DFK_SPONGE_CHECK_INVARIANTS(sponge) dfk__sponge_check_invariants((sponge))
#endif /* NDEBUG */

void dfk__sponge_init(dfk__sponge_t* sponge, dfk_t* dfk)
{
  assert(sponge);
  assert(dfk);
  sponge->base = NULL;
  sponge->cur = NULL;
  sponge->size = 0;
  sponge->capacity = 0;
  sponge->dfk = dfk;
}

void dfk__sponge_free(dfk__sponge_t* sponge)
{
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  if (sponge->base) {
    dfk__free(sponge->dfk, sponge->base);
  }
}

int dfk__sponge_write(dfk__sponge_t* sponge, char* buf, size_t nbytes)
{
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  assert(buf);
  assert(nbytes);
  if (!sponge->capacity) {
    /* Empty sponge - perform initial allocation */
    size_t toalloc = DFK_MAX(nbytes, DFK_SPONGE_INITIAL_SIZE);
    sponge->base = dfk__malloc(sponge->dfk, toalloc);
    if (!sponge->base) {
      return dfk_err_nomem;
    }
    sponge->cur = sponge->base;
    sponge->size = nbytes;
    sponge->capacity = toalloc;
    memcpy(sponge->base, buf, nbytes);
  } else if (nbytes <= sponge->capacity - sponge->size) {
    /* Lucky - use available space, no relocations needed */
    memcpy(sponge->base + sponge->size, buf, nbytes);
    sponge->size += nbytes;
  } else {
    /* Need reallocation */
    size_t nused = sponge->base + sponge->size - sponge->cur;
    size_t newsize = sponge->capacity + DFK_MAX(nused + nbytes, sponge->capacity);
    char* newbase = dfk__malloc(sponge->dfk, newsize);
    if (!newbase) {
      return dfk_err_nomem;
    }
    memcpy(newbase, sponge->cur, nused);
    memcpy(newbase + nused, buf, nbytes);
    dfk__free(sponge->dfk, sponge->base);
    sponge->base = newbase;
    sponge->cur = newbase;
    sponge->capacity = newsize;
    sponge->size = nused + nbytes;
  }
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return dfk_err_ok;
}

int dfk__sponge_writev(dfk__sponge_t* sponge, dfk_iovec_t* iov, size_t niov)
{
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  assert(iov);
  assert(niov);
  for (size_t i = 0; i < niov; ++i) {
    DFK_CALL(sponge->dfk, dfk__sponge_write(sponge, iov[i].data, iov[i].size));
  }
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return dfk_err_ok;
}

ssize_t dfk__sponge_read(dfk__sponge_t* sponge, char* buf, size_t nbytes)
{
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  assert(buf);
  assert(nbytes);
  if (!sponge->size) {
    return 0;
  }
  size_t toread = DFK_MIN(nbytes, (size_t) (sponge->base + sponge->size - sponge->cur));
  memcpy(buf, sponge->cur, toread);
  sponge->cur += toread;
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return toread;
}

ssize_t dfk__sponge_readv(dfk__sponge_t* sponge, dfk_iovec_t* iov, size_t niov)
{
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  assert(iov);
  assert(niov);
  ssize_t totalnread = 0;
  for (size_t i = 0; i < niov; ++i) {
    ssize_t nread = dfk__sponge_read(sponge, iov[i].data, iov[i].size);
    totalnread += nread;
    if (!nread) {
      break;
    }
  }
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return totalnread;
}

