#include <dfk/bufman.h>
#include "ut.h"

typedef struct {
  dfk_bufman_t bm;
  dfk_buf_t buf;
  dfk_bufman_alloc_request_t req;
} fixture;

static void fixture_setup(fixture* f)
{
  EXPECT(dfk_bufman_init(&f->bm) == 0);
  f->req.lifetime = 0;
  f->req.size = 100;
  f->req.usage = "fun";
  f->req.usagelen = 3;
  EXPECT(dfk_bufman_alloc(&f->bm, &f->req, &f->buf) == 0);
}

static void fixture_teardown(fixture* f)
{
  EXPECT(dfk_bufman_release(&f->bm, &f->buf) == 0);
  EXPECT(dfk_bufman_free(&f->bm) == 0);
}

TEST_F(buffer_manager, plain_alloc)
{
  size_t i;
  ASSERT(fixture->buf.size >= fixture->req.size);
  for (i = 0; i < fixture->req.size; ++i) {
    (void) fixture->buf.data[i];
  }
}

