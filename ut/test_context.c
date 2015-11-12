#include <dfk/context.h>

#include "ut.h"

TEST(context, default)
{
  dfk_context_t* ctx = dfk_default_context();
  char* allocated = NULL;

  ASSERT(ctx != NULL);
  allocated = ctx->malloc(100);
  EXPECT(allocated != NULL);
  allocated = ctx->realloc(allocated, 1000);
  EXPECT(allocated != NULL);
  ctx->free(allocated);
  EXPECT(ctx->logger != NULL);
  EXPECT(ctx->bufman_housekeeping_buffer_size != 0);
}
