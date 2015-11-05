#pragma once
#include <stdint.h>

#include <dfk/config.h>
#include <dfk/buf.h>
#include <dfk/context.h>

typedef struct {
  struct {
    void* hk;
  } _;
  dfk_context_t* context;
  int do_memzero;
  int do_cache;
} dfk_bufman_t;


typedef struct {
  size_t size;
  int64_t lifetime;
  const char* usage;
  size_t usagelen;
} dfk_bufman_alloc_request_t;


int dfk_bufman_init(dfk_bufman_t* bm);
int dfk_bufman_free(dfk_bufman_t* bm);
int dfk_bufman_alloc(dfk_bufman_t* bm, dfk_bufman_alloc_request_t* req, dfk_buf_t* buf);
int dfk_bufman_release(dfk_bufman_t* bm, dfk_buf_t* buf);
int dfk_bufman_tick(dfk_bufman_t* bm, int64_t now);
int dfk_bufman_for_each(dfk_bufman_t* bm, int (*functor)(dfk_bufman_t*, dfk_buf_t*, void*), void* userdata);
int dfk_bufman_lifetime(dfk_bufman_t* bm, dfk_buf_t* buf, int64_t* lifetime);
int dfk_bufman_usage(dfk_bufman_t* bm, dfk_buf_t* buf, const char** usage);

