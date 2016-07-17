/**
 * @file dfk/internal/sponge.h
 * In-memory byte stream
 *
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

