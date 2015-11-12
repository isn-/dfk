#include <assert.h>
#include <string.h>
#include <dfk/error.h>
#include "bufman_hk.h"

#define BUF_DEFAULT_USAGE "default"

int dfk_bufman_hk_alloc(dfk_context_t* ctx, dfk_bufman_hk_t** hk)
{
  dfk_bufman_hk_t* newhk;
  assert(ctx);
  assert(hk);
  newhk = (dfk_bufman_hk_t*) ctx->malloc(
    ctx->bufman_housekeeping_buffer_size);
  if (newhk == NULL) {
    return dfk_err_out_of_memory;
  }
  newhk->next = NULL;
  newhk->prev = NULL;
  newhk->self.buf.data = (char*) newhk;
  newhk->self.buf.size = ctx->bufman_housekeeping_buffer_size;
  newhk->self.lifetime = -1;
  newhk->self.used = 1;
  memcpy(newhk->self.usage, BUF_DEFAULT_USAGE, sizeof(BUF_DEFAULT_USAGE));
  newhk->nbuffers = 0;
  *hk = newhk;
  return dfk_err_ok;
}

int dfk_bufman_hk_free(dfk_context_t* ctx, dfk_bufman_hk_t* hk)
{
  dfk_bufman_hk_t* next;
  size_t i;
  while (hk) {
    next = hk->next;
    assert(hk->self.used == 1);
    ctx->free(hk->self.buf.data);
    for (i = 0; i < hk->nbuffers; ++i) {
      if (hk->buffers[i].used) {
        ctx->free(hk->buffers[i].buf.data);
      }
    }
    hk = next;
  }
  return dfk_err_ok;
}

int dfk_bufman_hk_empty(dfk_bufman_hk_t* hk)
{
  assert(hk);
  if (hk->self.buf.size < hk->nbuffers * sizeof(dfk_buf_ex_t) + sizeof(dfk_bufman_hk_t) - 2 * sizeof(dfk_buf_ex_t)) {
    return 1;
  }
  return 0;
}

