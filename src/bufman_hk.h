#pragma once
#include <stddef.h>
#include <stdint.h>
#include <dfk/buf.h>
#include <dfk/context.h>


typedef struct {
  int64_t lifetime;
  char usage[8];
  dfk_buf_t buf;
  int used;
} dfk_buf_ex_t;


typedef struct dfk_bufman_hk_t {
  struct dfk_bufman_hk_t* next;
  struct dfk_bufman_hk_t* prev;
  dfk_buf_ex_t self;
  size_t nbuffers;
  dfk_buf_ex_t buffers[1];
} dfk_bufman_hk_t;


int dfk_bufman_hk_alloc(dfk_context_t* ctx, dfk_bufman_hk_t** hk);
int dfk_bufman_hk_free(dfk_context_t* ctx, dfk_bufman_hk_t* hk);
int dfk_bufman_hk_empty(dfk_bufman_hk_t* hk);

