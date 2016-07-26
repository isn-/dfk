/**
 * @file dfk/internal/arena.h
 * An effective small object allocator
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
#include <dfk/config.h>
#include <dfk/core.h>
#include <dfk/internal/list.h>


typedef struct dfk_arena_t {
  struct {
    dfk_list_t segments;
    dfk_list_t owc; /* objects with cleanup */
  } _;
  dfk_t* dfk;
} dfk_arena_t;


int dfk_arena_init(dfk_arena_t* arena, dfk_t* dfk);
int dfk_arena_free(dfk_arena_t* arena);

void* dfk_arena_alloc(dfk_arena_t* arena, size_t size);
void* dfk_arena_alloc_copy(dfk_arena_t* arena, const char* data, size_t size);

typedef void (*dfk_arena_cleanup)(dfk_arena_t*, void*);
void* dfk_arena_alloc_ex(dfk_arena_t* arena, size_t size, dfk_arena_cleanup clean);

