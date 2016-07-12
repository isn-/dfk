/**
 * @copyright
 * Copyright (c) 2016, Stanislav Ivochkin. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <dfk.h>
#include <dfk/internal.h>
#include <dfk/internal/sponge.h>
#include "ut.h"


typedef struct fixture_t {
  dfk_t dfk;
  dfk_sponge_t sponge;
} fixture_t;


static void fixture_setup(fixture_t* f)
{
  dfk_init(&f->dfk);
  dfk_sponge_init(&f->sponge, &f->dfk);
}


static void fixture_teardown(fixture_t* f)
{
  dfk_sponge_free(&f->sponge);
  dfk_free(&f->dfk);
}


TEST_F(fixture, sponge, write_init)
{
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, "foo", 3));
  EXPECT(fixture->sponge.size == 3);
  EXPECT(fixture->sponge.cur == fixture->sponge.base);
  EXPECT(!memcmp(fixture->sponge.base, "foo", 3));
}


TEST_F(fixture, sponge, write_no_alloc)
{
  EXPECT(DFK_SPONGE_INITIAL_SIZE >= 2);
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, "a", 1));
  EXPECT(fixture->sponge.size == 1);
  EXPECT(fixture->sponge.cur == fixture->sponge.base);
  EXPECT(!memcmp(fixture->sponge.base, "a", 1));
  char* base = fixture->sponge.base;
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, "b", 1));
  EXPECT(base == fixture->sponge.base);
  EXPECT(fixture->sponge.size == 2);
  EXPECT(!memcmp(fixture->sponge.base, "ab", 2));
}


TEST_F(fixture, sponge, write_realloc)
{
  char largebuf[DFK_SPONGE_INITIAL_SIZE - 1] = {0};
  for (size_t i = 0; i < DFK_SIZE(largebuf); ++i) {
    largebuf[i] = (char) i;
  }
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, largebuf, sizeof(largebuf)));
  EXPECT(fixture->sponge.capacity > fixture->sponge.size);
  char* base = fixture->sponge.base;
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, largebuf, sizeof(largebuf)));
  EXPECT(base != fixture->sponge.base);
  EXPECT(fixture->sponge.size = 2 * (DFK_SPONGE_INITIAL_SIZE - 1));
  EXPECT(fixture->sponge.cur == fixture->sponge.base);
  EXPECT(!memcmp(fixture->sponge.cur, largebuf, sizeof(largebuf)));
  EXPECT(!memcmp(fixture->sponge.cur + sizeof(largebuf), largebuf, sizeof(largebuf)));
}


static void* out_of_memory(dfk_t* p, size_t size)
{
  int* oom = (int*) p->user.data;
  if (!oom || *oom) {
    return NULL;
  }
  return malloc(size);
}


TEST_F(fixture, sponge, write_init_no_mem)
{
  int oom = 1;
  fixture->dfk.user.data = &oom;
  fixture->dfk.malloc = out_of_memory;
  EXPECT(dfk_sponge_write(&fixture->sponge, "foo", 3) == dfk_err_nomem);
}


TEST_F(fixture, sponge, write_realloc_no_mem)
{
  int oom = 0;
  char largebuf[DFK_SPONGE_INITIAL_SIZE - 1] = {0};
  fixture->dfk.user.data = &oom;
  fixture->dfk.malloc = out_of_memory;
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, largebuf, sizeof(largebuf)));
  oom = 1;
  EXPECT(dfk_sponge_write(&fixture->sponge, largebuf, sizeof(largebuf)) == dfk_err_nomem);
}


TEST_F(fixture, sponge, writev)
{
  char largebuf[(DFK_SPONGE_INITIAL_SIZE - 1)/ 2] = {0};
  dfk_iovec_t iov[] = {
    {largebuf, sizeof(largebuf)},
    {largebuf, sizeof(largebuf)},
    {largebuf, sizeof(largebuf)},
  };
  EXPECT_OK(dfk_sponge_writev(&fixture->sponge, iov, DFK_SIZE(iov)));
  EXPECT(fixture->sponge.size == DFK_SIZE(iov) * sizeof(largebuf));
  for (size_t i = 0; i < DFK_SIZE(iov); ++i) {
    EXPECT(!memcmp(fixture->sponge.base + i * sizeof(largebuf), largebuf, sizeof(largebuf)));
  }
}


TEST_F(fixture, sponge, writev_no_mem)
{
  dfk_iovec_t iov[] = {
    {"foo", 3},
    {"quax", 4}
  };
  fixture->dfk.malloc = out_of_memory;
  EXPECT(dfk_sponge_writev(&fixture->sponge, iov, DFK_SIZE(iov)) == dfk_err_nomem);
}


TEST_F(fixture, sponge, read_empty)
{
  char buf[1];
  EXPECT(dfk_sponge_read(&fixture->sponge, buf, 1) == 0);
}


TEST_F(fixture, sponge, read_enough)
{
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, "foobar", 6));
  char buf[4];
  EXPECT(dfk_sponge_read(&fixture->sponge, buf, sizeof(buf)) == 4);
  EXPECT(!memcmp(buf, "foob", 4));
  EXPECT(fixture->sponge.size == 6);
  EXPECT(fixture->sponge.base + fixture->sponge.size - fixture->sponge.cur == 2);
}


TEST_F(fixture, sponge, read_more)
{
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, "foobar", 6));
  char buf[10];
  EXPECT(dfk_sponge_read(&fixture->sponge, buf, sizeof(buf)) == 6);
  EXPECT(!memcmp(buf, "foobar", 6));
}


TEST_F(fixture, sponge, read_exhausted)
{
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, "foobar", 6));
  char buf[10];
  EXPECT(dfk_sponge_read(&fixture->sponge, buf, sizeof(buf)) == 6);
  EXPECT(!memcmp(buf, "foobar", 6));
  EXPECT(dfk_sponge_read(&fixture->sponge, buf, sizeof(buf)) == 0);
}


TEST_F(fixture, sponge, readv_empty)
{
  char buf[10];
  dfk_iovec_t iov[] = {
    {buf, 5},
    {buf + 5, 5}
  };
  EXPECT(dfk_sponge_readv(&fixture->sponge, iov, DFK_SIZE(iov)) == 0);
}


TEST_F(fixture, sponge, readv_all)
{
  char buf[10];
  dfk_iovec_t iov[] = {
    {buf, 5},
    {buf + 5, 5}
  };
  EXPECT_OK(dfk_sponge_write(&fixture->sponge, "foobar", 6));
  EXPECT(dfk_sponge_readv(&fixture->sponge, iov, DFK_SIZE(iov)) == 6);
  EXPECT(!memcmp(buf, "foobar", 6));
  EXPECT(dfk_sponge_readv(&fixture->sponge, iov, DFK_SIZE(iov)) == 0);
}

