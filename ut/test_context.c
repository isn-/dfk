#include <dfk/context.h>
#include "common.h"
#include "ut.h"

TEST(context, default)
{
  dfk_context_t* ctx = dfk_default_context();
  char* allocated = NULL;

  ASSERT(ctx != NULL);
  allocated = DFK_MALLOC(ctx, 100);
  EXPECT(allocated != NULL);
  allocated = DFK_REALLOC(ctx, allocated, 1000);
  EXPECT(allocated != NULL);
  DFK_FREE(ctx, allocated);
  EXPECT(ctx->log != NULL);
}
