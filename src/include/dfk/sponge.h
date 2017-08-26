/**
 * @file dfk/sponge.h
 * In-memory byte stream
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <sys/types.h>
#include <dfk/context.h>

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
typedef struct dfk__sponge_t {
  char* base;
  char* cur;
  size_t size;
  size_t capacity;
  dfk_t* dfk;
} dfk__sponge_t;


void dfk__sponge_init(dfk__sponge_t* sponge, dfk_t* dfk);
void dfk__sponge_free(dfk__sponge_t* sponge);
int dfk__sponge_write(dfk__sponge_t* sponge, char* buf, size_t nbytes);
int dfk__sponge_writev(dfk__sponge_t* sponge, dfk_iovec_t* iov, size_t niov);
ssize_t dfk__sponge_read(dfk__sponge_t* sponge, char* buf, size_t nbytes);
ssize_t dfk__sponge_readv(dfk__sponge_t* sponge, dfk_iovec_t* iov, size_t niov);

