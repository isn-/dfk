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
#include <string.h>
#include <dfk/config.h>
#include <dfk/internal.h>
#include <dfk/internal/sponge.h>


#ifdef NDEBUG
#define DFK_SPONGE_CHECK_INVARIANTS(sponge) DFK_UNUSED(sponge)
#else

static void dfk__sponge_check_invariants(dfk_sponge_t* sponge)
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


void dfk_sponge_init(dfk_sponge_t* sponge, dfk_t* dfk)
{
  assert(sponge);
  assert(dfk);
  memset(sponge, 0, sizeof(*sponge));
  sponge->dfk = dfk;
}


void dfk_sponge_free(dfk_sponge_t* sponge)
{
  assert(sponge);
  DFK_FREE(sponge->dfk, sponge->base);
}


int dfk_sponge_write(dfk_sponge_t* sponge, char* buf, size_t nbytes)
{
  assert(sponge);
  assert(buf || !nbytes);
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  if (!sponge->capacity) {
    /* Empty sponge - perform initial allocation */
    size_t toalloc = DFK_MAX(nbytes, DFK_SPONGE_INITIAL_SIZE);
    sponge->base = DFK_MALLOC(sponge->dfk, toalloc);
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
    char* newbase = DFK_MALLOC(sponge->dfk, newsize);
    if (!newbase) {
      return dfk_err_nomem;
    }
    memcpy(newbase, sponge->cur, nused);
    memcpy(newbase + nused, buf, nbytes);
    DFK_FREE(sponge->dfk, sponge->base);
    sponge->base = newbase;
    sponge->cur = newbase;
    sponge->capacity = newsize;
    sponge->size = nused + nbytes;
  }
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return dfk_err_ok;
}


int dfk_sponge_writev(dfk_sponge_t* sponge, dfk_iovec_t* iov, size_t niov)
{
  assert(sponge);
  assert(iov || !niov);
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  for (size_t i = 0; i < niov; ++i) {
    DFK_CALL(sponge->dfk, dfk_sponge_write(sponge, iov[i].data, iov[i].size));
  }
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return dfk_err_ok;
}


ssize_t dfk_sponge_read(dfk_sponge_t* sponge, char* buf, size_t nbytes)
{
  assert(sponge);
  assert(buf || !nbytes);
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  if (!sponge->size) {
    return 0;
  }
  size_t toread = DFK_MIN(nbytes, (size_t) (sponge->base + sponge->size - sponge->cur));
  memcpy(buf, sponge->cur, toread);
  sponge->cur += toread;
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return toread;
}


ssize_t dfk_sponge_readv(dfk_sponge_t* sponge, dfk_iovec_t* iov, size_t niov)
{
  assert(sponge);
  assert(iov || !niov);
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  ssize_t totalnread = 0;
  for (size_t i = 0; i < niov; ++i) {
    ssize_t nread = dfk_sponge_read(sponge, iov[i].data, iov[i].size);
    totalnread += nread;
    if (!nread) {
      break;
    }
  }
  DFK_SPONGE_CHECK_INVARIANTS(sponge);
  return totalnread;
}

