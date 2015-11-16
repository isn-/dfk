#include <assert.h>
#include <string.h>
#include <dfk/error.h>
#include <dfk/bufman.h>
#include "bufman_hk.h"

int dfk_bufman_init(dfk_bufman_t* bm)
{
  assert(bm);
  bm->_.hk = NULL;
  bm->do_memzero = 1;
  bm->do_cache = 0;
  bm->context = dfk_default_context();
  return dfk_err_ok;
}

int dfk_bufman_free(dfk_bufman_t* bm)
{
  dfk_bufman_hk_t* hk, *hkcopy;
  assert(bm);
  hk = bm->_.hk;
  while (hk) {
    size_t i;
    for (i = 0; i < hk->nbuffers; ++i) {
      if (!hk->buffers[i].released) {
        bm->context->free(hk->buffers[i].buf.data);
      }
    }
    hkcopy = hk;
    hk = hk->next;
    bm->context->free(hkcopy->self.buf.data);
  }
#ifdef DFK_ENABLE_CLEANUP
  bm->do_memzero = -1;
  bm->do_cache = -1;
  bm->context = NULL;
#endif
  return dfk_err_ok;
}

static int alloc_new_hk(dfk_context_t* context, dfk_buf_t* hkbuf, dfk_buf_ex_t** found_buffer)
{
  dfk_bufman_hk_t* hk;
  int err;
  assert(hkbuf);
  assert(found_buffer);
  hkbuf->data = context->malloc(hkbuf->size);
  if (hkbuf->data == NULL) {
    return dfk_err_out_of_memory;
  }
  hk = (dfk_bufman_hk_t*) hkbuf->data;
  if ((err = dfk_bufman_hk_init(hk, hkbuf)) != dfk_err_ok) {
    return err;
  }
  hk->nbuffers = 1;
  *found_buffer = &hk->buffers[0];
  if ((err = dfk_buf_ex_init(*found_buffer)) != dfk_err_ok) {
    return err;
  }
  return dfk_err_ok;
}

int dfk_bufman_alloc(dfk_bufman_t* bm, dfk_bufman_alloc_request_t* req, dfk_buf_t** outbuf)
{
  size_t size, usagelen;
  int err;
  int hk_action = -1;
  dfk_buf_t hkbuf = {NULL, DFK_BUFFER_MANAGER_HOUSEKEEPING_SIZE};
  dfk_buf_ex_t* found_buffer = NULL;

  assert(bm);
  assert(req);
  assert(outbuf);

  /* Align buffer to the page size
   */
  size = DFK_PAGE_SIZE * (1 + (req->size / DFK_PAGE_SIZE));

  /* First time initialization.
   * No housekeeping has ever been previously allocated.
   * Create a new housekeeping, save pointer to the first
   * buffer into found_buffer variable.
   */
  if (!bm->_.hk) {
    if ((err = alloc_new_hk(bm->context, &hkbuf, &found_buffer)) != dfk_err_ok) {
      goto dfk_bufman_alloc_cleanup;
    }
    hk_action = 1;
  } else {
    /* Find existing buffer, or allocate a new housekeeping if
     * all others are empty
     */
    dfk_bufman_hk_t* hk = bm->_.hk;
    size_t i;
    while (!found_buffer && hk) {
      for (i = 0; i < hk->nbuffers; ++i) {
        if (!hk->buffers[i].used && hk->buffers[i].buf.size >= size) {
          found_buffer = &hk->buffers[i];
          hk_action = 0;
          break;
        }
      }
      hk = hk->next;
    }
    if (!found_buffer) {
      /* All buffers are used - allocate a new housekeeping,
       * append it to the end of housekeeping list
       */
      if ((err = alloc_new_hk(bm->context, &hkbuf, &found_buffer)) != dfk_err_ok) {
        goto dfk_bufman_alloc_cleanup;
      }
      hk_action = 2;
    }
  }

  assert(hk_action != -1);

  /* Fill found_buffer */
  assert(found_buffer);
  if (!found_buffer->buf.data) {
    /* Need to allocate memory */
    assert(found_buffer->buf.data == NULL);
    found_buffer->buf.data = bm->context->malloc(size);
    if (found_buffer->buf.data == NULL) {
      err = dfk_err_out_of_memory;
      goto dfk_bufman_alloc_cleanup;
    }
    found_buffer->buf.size = size;
  }
  found_buffer->lifetime = req->lifetime;
  memset(found_buffer->usage, 0, sizeof(found_buffer->usage));
  usagelen = req->usagelen < sizeof(found_buffer->usage)
      ? req->usagelen
      : sizeof(found_buffer->usage) - 1;
  memcpy(found_buffer->usage, req->usage, usagelen);
  found_buffer->used |= 1;

  /* Perform housekeeping action */
  switch (hk_action) {
    case 0: { /* no action - found existing buffer */
      break;
    }
    case 1: { /* first time allocation */
      bm->_.hk = (dfk_bufman_hk_t*) hkbuf.data;
      break;
    }
    case 2: { /* append new housekeeping */
      dfk_bufman_hk_t* tail = bm->_.hk;
      while (tail->next) {
        tail = tail->next;
      }
      assert(hkbuf.data);
      tail->next = (dfk_bufman_hk_t*) hkbuf.data;
      tail->next->prev = tail;
      break;
    }
    default: {
      assert(0 && "no such hk_action" && hk_action);
    }
  }

  /* Set result */
  *outbuf = &found_buffer->buf;
  return dfk_err_ok;

dfk_bufman_alloc_cleanup:
  if (hkbuf.data) {
    bm->context->free(hkbuf.data);
  }
  return err;
}

int dfk_bufman_release(dfk_bufman_t* bm, dfk_buf_t* buf)
{
  dfk_buf_ex_t* bufex = (dfk_buf_ex_t*) buf;
  assert(bm);
  assert(buf);
  if (!dfk_buf_ex_valid(bufex)) {
    return dfk_err_badarg;
  }
  if (bm->do_cache) {
    bufex->used = 0;
  } else {
    bufex->released = 1;
    bm->context->free(buf->data);
  }
  return dfk_err_ok;
}

static int for_each(dfk_bufman_t* bm, int (*functor)(dfk_bufman_t*, dfk_bufman_hk_t*, dfk_buf_ex_t*, void*), void* userdata)
{
  dfk_bufman_hk_t* hk;
  int err;
  size_t i;

  assert(bm);
  assert(functor);

  hk = bm->_.hk;
  while (hk) {
    for (i = 0; i < hk->nbuffers; ++i) {
      if ((err = functor(bm, hk, &hk->buffers[i], userdata)) != dfk_err_ok) {
        return err;
      }
    }
    hk = hk->next;
  }

  return dfk_err_ok;
}

static int tick_functor(dfk_bufman_t* bm, dfk_bufman_hk_t* hk, dfk_buf_ex_t* buf, void* userdata)
{
  int err;
  int64_t* now = userdata;
  (void) hk;
  if (buf->lifetime < *now) {
    if ((err = dfk_bufman_release(bm, &buf->buf)) != dfk_err_ok) {
      return err;
    }
  }
  return dfk_err_ok;
}

int dfk_bufman_tick(dfk_bufman_t* bm, int64_t now)
{
  assert(bm);
  return for_each(bm, tick_functor, &now);
}

typedef struct {
  int (*functor)(dfk_bufman_t*, dfk_buf_t*, void*);
  void* userdata;
} for_each_arg;

static int for_each_functor(dfk_bufman_t* bm, dfk_bufman_hk_t* hk, dfk_buf_ex_t* buf, void* userdata)
{
  for_each_arg* arg = userdata;
  (void) hk;
  return arg->functor(bm, &buf->buf, arg->userdata);
}

int dfk_bufman_for_each(dfk_bufman_t* bm, int (*functor)(dfk_bufman_t*, dfk_buf_t*, void*), void* userdata)
{
  for_each_arg ud;
  ud.functor = functor;
  ud.userdata = userdata;
  assert(bm);
  assert(functor);
  return for_each(bm, &for_each_functor, &ud);
}

int dfk_bufman_lifetime(dfk_bufman_t* bm, dfk_buf_t* buf, int64_t* lifetime)
{
  dfk_buf_ex_t* bufex = (dfk_buf_ex_t*) buf;
  (void) bm;
  if (!dfk_buf_ex_valid(bufex)) {
    return dfk_err_badarg;
  }
  *lifetime = bufex->lifetime;
  return dfk_err_ok;
}

int dfk_bufman_usage(dfk_bufman_t* bm, dfk_buf_t* buf, const char** usage)
{
  dfk_buf_ex_t* bufex = (dfk_buf_ex_t*) buf;
  (void) bm;
  if (!dfk_buf_ex_valid(bufex)) {
    return dfk_err_badarg;
  }
  *usage = bufex->usage;
  return dfk_err_ok;
}

