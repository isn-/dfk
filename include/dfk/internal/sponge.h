/**
 * @file dfk/internal/sponge.h
 * In-memory byte stream
 *
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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

