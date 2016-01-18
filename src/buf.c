#include <dfk/core.h>
#include <dfk/buf.h>
#include "common.h"

int dfk_buf_init(dfk_buf_t* buf, dfk_context_t* ctx, size_t nbytes)
{
  if (buf == NULL) {
    return dfk_err_badarg;
  }
  buf->_.context = ctx;
  buf->size = nbytes;
  buf->data = DFK_MALLOC(ctx, nbytes);
  if (buf->data == NULL) {
    return dfk_err_out_of_memory;
  }
  return dfk_err_ok;
}

int dfk_buf_free(dfk_buf_t* buf)
{
  if (buf == NULL) {
    return dfk_err_badarg;
  }
  DFK_FREE(buf->_.context, buf->data);
  return dfk_err_ok;
}

