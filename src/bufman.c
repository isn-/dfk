#include <assert.h>
#include <string.h>
#include <dfk/error.h>
#include <dfk/bufman.h>
#include "bufman_hk.h"

int dfk_bufman_init(dfk_bufman_t* bm)
{
  bm->_.hk = NULL;
  bm->do_memzero = 1;
  bm->do_cache = 0;
  bm->context = dfk_default_context();
  return dfk_err_ok;
}

int dfk_bufman_free(dfk_bufman_t* bm)
{
  assert(bm);
  return dfk_bufman_hk_free(bm->context, bm->_.hk);
}

int dfk_bufman_alloc(dfk_bufman_t* bm, dfk_bufman_alloc_request_t* req, dfk_buf_t* buf)
{
  dfk_bufman_hk_t* hk;
  int housekeeping_is_new = 0;
  char* new_buf;
  size_t usagelen;
  size_t size = DFK_PAGE_SIZE * (1 + (req->size / DFK_PAGE_SIZE));
  int err;

  assert(bm);
  assert(req);
  assert(buf);

  if (!bm->_.hk || dfk_bufman_hk_empty(bm->_.hk)) {
    if ((err = dfk_bufman_hk_alloc(bm->context, &hk)) != dfk_err_ok) {
      return err;
    }
    housekeeping_is_new = 1;
  } else {
    hk = bm->_.hk;
  }
  new_buf = bm->context->malloc(size);
  if (!new_buf) {
    if (housekeeping_is_new) {
      dfk_bufman_hk_free(bm->context, hk);
    }
    return dfk_err_out_of_memory;
  }
  if (bm->do_memzero) {
    memset(new_buf, '\0', size);
  }
  hk->buffers[hk->nbuffers].buf.data = new_buf;
  hk->buffers[hk->nbuffers].buf.size = size;
  hk->buffers[hk->nbuffers].lifetime = req->lifetime;
  hk->buffers[hk->nbuffers].used = 1;
  usagelen = req->usagelen < 8 ? req->usagelen : 7;
  memcpy(hk->buffers[hk->nbuffers].usage, req->usage, usagelen);
  hk->buffers[hk->nbuffers].usage[usagelen] = '\0';
  hk->nbuffers = hk->nbuffers + 1;

  if (housekeeping_is_new) {
    if (bm->_.hk) {
      ((dfk_bufman_hk_t*) bm->_.hk)->prev = hk;
      hk->next = bm->_.hk;
    }
    bm->_.hk = hk;
  }

  buf->data = new_buf;
  buf->size = size;

  return dfk_err_ok;
}

static int bufman_for_each(dfk_bufman_t* bm, int (*functor)(dfk_bufman_t*, dfk_bufman_hk_t*, dfk_buf_ex_t*, void*), void* userdata)
{
  dfk_bufman_hk_t* hk;
  int err;
  size_t i;

  assert(bm);
  assert(functor);

  hk = bm->_.hk;
  while (hk) {
    if ((err = functor(bm, hk, &hk->self, userdata)) != dfk_err_ok) {
      return err;
    }
    for (i = 0; i < hk->nbuffers; ++i) {
      if ((err = functor(bm, hk, &hk->buffers[i], userdata)) != dfk_err_ok) {
        return err;
      }
    }
    hk = hk->next;
  }

  return dfk_err_ok;
}

static int release_functor(dfk_bufman_t* bm, dfk_bufman_hk_t* hk, dfk_buf_ex_t* buf, void* userdata)
{
  size_t i;
  int err;
  (void) hk;
  if (&buf->buf == userdata) {
    buf->used = 0;
    bm->context->free(buf->buf.data);
    if (dfk_bufman_hk_empty(hk)) {
      for (i = 0; i < hk->nbuffers; ++i) {
        if (hk->buffers[i].used) {
          break;
        }
      }
      if (hk->prev) {
        ((dfk_bufman_hk_t*) hk->prev)->next = hk->next;
      }
      if (hk->next) {
        ((dfk_bufman_hk_t*) hk->next)->prev = hk->prev;
      }
      if ((err = dfk_bufman_hk_free(bm->context, hk)) != dfk_err_ok) {
        return err;
      }
    }
    return -1;
  }
  return dfk_err_ok;
}

int dfk_bufman_release(dfk_bufman_t* bm, dfk_buf_t* buf)
{
  assert(bm);
  assert(buf);
  if (bufman_for_each(bm, release_functor, buf) == -1) {
    return dfk_err_ok;
  }
  return dfk_err_not_found;
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
  return bufman_for_each(bm, tick_functor, &now);
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
  return bufman_for_each(bm, &for_each_functor, &ud);
}

typedef struct {
  dfk_buf_t* buf;
  int64_t* lifetime;
} lifetime_arg;

static int lifetime_functor(dfk_bufman_t* bm, dfk_bufman_hk_t* hk, dfk_buf_ex_t* buf, void* userdata)
{
  lifetime_arg* arg = userdata;
  (void) bm;
  (void) hk;
  if (&buf->buf == arg->buf) {
    *arg->lifetime= buf->lifetime;
    return 1;
  }
  return dfk_err_ok;
}

int dfk_bufman_lifetime(dfk_bufman_t* bm, dfk_buf_t* buf, int64_t* lifetime)
{
  lifetime_arg arg;
  arg.buf = buf;
  arg.lifetime = lifetime;
  assert(bm);
  assert(buf);
  assert(lifetime);
  if (bufman_for_each(bm, lifetime_functor, &arg) == 1) {
    return dfk_err_ok;
  }
  return dfk_err_not_found;
}

typedef struct {
  dfk_buf_t* buf;
  const char** usage;
} usage_arg;

static int usage_functor(dfk_bufman_t* bm, dfk_bufman_hk_t* hk, dfk_buf_ex_t* buf, void* userdata)
{
  usage_arg* arg = userdata;
  (void) bm;
  (void) hk;
  if (&buf->buf == arg->buf) {
    *arg->usage = buf->usage;
    return 1;
  }
  return dfk_err_ok;
}

int dfk_bufman_usage(dfk_bufman_t* bm, dfk_buf_t* buf, const char** usage)
{
  usage_arg arg;
  arg.buf = buf;
  arg.usage = usage;
  assert(bm);
  assert(buf);
  assert(usage);
  if (bufman_for_each(bm, usage_functor, &arg) == 1) {
    return dfk_err_ok;
  }
  return dfk_err_not_found;
}

