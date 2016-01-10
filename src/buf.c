#include <assert.h>
#include <dfk/buf.h>
#include <dfk/error.h>

int dfk_buf_init(dfk_buf_t* buf, dfk_context_t* ctx, size_t nbytes)
{
  assert(buf);
  buf->_.context = ctx;
  buf->size = nbytes;
  buf->data = ctx->malloc(ctx->userdata, nbytes);
  if (buf->data == NULL) {
    return dfk_err_out_of_memory;
  }
  return dfk_err_ok;
}

int dfk_buf_free(dfk_buf_t* buf)
{
  assert(buf);
  buf->_.context->free(buf->_.context->userdata, buf->data);
  return dfk_err_ok;
}
