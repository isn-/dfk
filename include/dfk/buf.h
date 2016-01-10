#pragma once
#include <stddef.h>
#include <dfk/context.h>

typedef struct {
  struct {
    dfk_context_t* context;
  } _;
  char* data;
  size_t size;
} dfk_buf_t;


int dfk_buf_init(dfk_buf_t* buf, dfk_context_t* ctx, size_t nbytes);
int dfk_buf_free(dfk_buf_t* buf);
