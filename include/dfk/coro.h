/**
 * @file dfk/coro.h
 * Coroutine object and related functions
 *
 * @copyright
 * Copyright (c) 2015, 2016, Stanislav Ivochkin. All Rights Reserved.
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <libcoro/coro.h>
#include <dfk/config.h>
#include <dfk/context.h>
#include <dfk/buf.h>


struct dfk_coro_t;

typedef struct dfk_coro_t {
  struct {
    dfk_context_t* context;
    struct coro_context ctx;
    struct dfk_coro_t* parent;
    void (*func)(void*);
    void* arg;
    char* stack;
    size_t stack_size;
    int terminated : 1;
  } _;
} dfk_coro_t;


int dfk_coro_init(dfk_coro_t* coro, dfk_context_t* context, size_t stack_size);
int dfk_coro_run(dfk_coro_t* coro, void (*func)(void*), void* arg);
int dfk_coro_yield(dfk_coro_t* from, dfk_coro_t* to);
int dfk_coro_yield_to(dfk_context_t* ctx, dfk_coro_t* to);
int dfk_coro_yield_parent(dfk_coro_t* from);
int dfk_coro_join(dfk_coro_t* coro);

