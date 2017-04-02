/**
 * @file dfk/internal/sponge.h
 * In-memory byte stream
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <dfk/core.h>

/**
 * In-memory byte stream
 *
 * Support read/write operations. Have the same meaning as io.BytesIO in python,
 * or std::stringstream in C++.
 *
 * Extensively used for testing.
 *
 * @warning Does a lot of allocations, therefore is not efficient
 */
typedef struct dfk_sponge_t {
  char* base;
  char* cur;
  size_t size;
  size_t capacity;
  dfk_t* dfk;
} dfk_sponge_t;


void dfk_sponge_init(dfk_sponge_t* sponge, dfk_t* dfk);
void dfk_sponge_free(dfk_sponge_t* sponge);
int dfk_sponge_write(dfk_sponge_t* sponge, char* buf, size_t nbytes);
int dfk_sponge_writev(dfk_sponge_t* sponge, dfk_iovec_t* iov, size_t niov);
ssize_t dfk_sponge_read(dfk_sponge_t* sponge, char* buf, size_t nbytes);
ssize_t dfk_sponge_readv(dfk_sponge_t* sponge, dfk_iovec_t* iov, size_t niov);

