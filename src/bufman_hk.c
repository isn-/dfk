#include <assert.h>
#include <string.h>
#include <dfk/error.h>
#include "bufman_hk.h"

#define BUF_DEFAULT_USAGE "default"
#define BUF_HK_USAGE "bufm.hk"
#define BUF_MAGIC 0xDEADBEEFDEADBEEF

int dfk_buf_ex_init(dfk_buf_ex_t* buf)
{
  assert(buf);
  buf->buf.data = NULL;
  buf->buf.size = 0;
#ifdef DFK_ENABLE_MAGIC
  buf->magic = BUF_MAGIC;
#endif
  buf->lifetime = -1;
  memcpy(buf->usage, BUF_DEFAULT_USAGE, sizeof(BUF_DEFAULT_USAGE));
  buf->used = 1;
  buf->released = 0;
  return dfk_err_ok;
}

int dfk_buf_ex_free(dfk_buf_ex_t* buf)
{
  assert(buf);
#ifdef DFK_ENABLE_CLEANUP
  buf->buf.data = NULL;
  buf->buf.size = 0;
  buf->lifetime = -2;
  buf->used =  1;
  buf->released =  1;
#ifdef DFK_ENABLE_MAGIC
  buf->magic = -1;
#endif
#endif
  return dfk_err_ok;
}

int dfk_buf_ex_valid(dfk_buf_ex_t* buf)
{
  assert(buf);
#ifdef DFK_ENABLE_MAGIC
  return buf->magic == BUF_MAGIC;
#else
  return 1;
#endif
}

int dfk_bufman_hk_init(dfk_bufman_hk_t* hk, dfk_buf_t* buf)
{
  int err;
  hk->next = NULL;
  hk->prev = NULL;
  hk->nbuffers = 0;
  if ((err = dfk_buf_ex_init(&hk->self)) != dfk_err_ok) {
    return err;
  }
  memcpy(hk->self.usage, BUF_HK_USAGE, sizeof(BUF_HK_USAGE));
  hk->self.buf.data = buf->data;
  hk->self.buf.size = buf->size;
  memset(hk->buffers, 0, sizeof(hk->buffers));
  return dfk_err_ok;
}

int dfk_bufman_hk_empty(dfk_bufman_hk_t* hk)
{
  assert(hk);
  return hk->self.buf.size <= sizeof(dfk_bufman_hk_t) + hk->nbuffers * sizeof(dfk_buf_ex_t);
}

