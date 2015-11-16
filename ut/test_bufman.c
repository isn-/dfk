#include <dfk.h>
#include "ut.h"

typedef struct {
  dfk_bufman_t bm;
  dfk_buf_t* buf;
  dfk_bufman_alloc_request_t req;
} fixture;

static void fixture_setup(fixture* f)
{
  EXPECT(dfk_bufman_init(&f->bm) == dfk_err_ok);
  f->req.lifetime = 10;
  f->req.size = 100;
  f->req.usage = "fun";
  f->req.usagelen = 3;
  EXPECT(dfk_bufman_alloc(&f->bm, &f->req, &f->buf) == dfk_err_ok);
}

static void fixture_teardown(fixture* f)
{
  if (f->buf) {
    EXPECT(dfk_bufman_release(&f->bm, f->buf) == dfk_err_ok);
  }
  EXPECT(dfk_bufman_free(&f->bm) == dfk_err_ok);
}

TEST_F(buffer_manager, plain_alloc)
{
  size_t i;
  ASSERT(fixture->buf->size >= fixture->req.size);
  /* Check that data is accessible. If fixture->buf points to
   * not valid chunk of memory, valgrind or asan will report
   * an error.
   */
  for (i = 0; i < fixture->req.size; ++i) {
    (void) fixture->buf->data[i];
  }
}

TEST_F(buffer_manager, lifetime)
{
  int64_t lifetime;
  EXPECT(dfk_bufman_lifetime(&fixture->bm, fixture->buf, &lifetime) == dfk_err_ok);
  EXPECT(lifetime == 10);
}

TEST_F(buffer_manager, cache)
{
  const char* first_buf = fixture->buf->data;
  dfk_buf_t* newbuf;
  fixture->bm.do_cache = 1;
  EXPECT(dfk_bufman_release(&fixture->bm, fixture->buf) == dfk_err_ok);
  fixture->req.usage = "joy";
  fixture->req.lifetime = 5;
  EXPECT(dfk_bufman_alloc(&fixture->bm, &fixture->req, &newbuf) == dfk_err_ok);
  EXPECT(newbuf->data == first_buf);
  EXPECT(dfk_bufman_release(&fixture->bm, newbuf) == dfk_err_ok);
  fixture->buf = NULL;
}

TEST_F(buffer_manager, 1k_ops)
{
  dfk_buf_t* bufs[1000];
  size_t i;
  for (i = 0; i < (sizeof(bufs) / sizeof(bufs[0])); ++i) {
    EXPECT(dfk_bufman_alloc(&fixture->bm, &fixture->req, &bufs[i]) == dfk_err_ok);
    EXPECT(bufs[i]->data != NULL);
    EXPECT(bufs[i]->size >= fixture->req.size);
  }
  for (i = 0; i < (sizeof(bufs) / sizeof(bufs[0])); ++i) {
    EXPECT(dfk_bufman_release(&fixture->bm, bufs[i]) == dfk_err_ok);
  }
}

