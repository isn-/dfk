/**
 * @file dfk/sync.h
 * Synchronization primitives
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

#include <dfk/config.h>
#include <dfk/core.h>
#include <dfk/internal/list.h>

typedef struct dfk_mutex_t {
  struct {
    dfk_list_t waitqueue;
    dfk_coro_t* owner;
  } _;
  dfk_t* dfk;
} dfk_mutex_t;

int dfk_mutex_init(dfk_mutex_t* mutex, dfk_t* dfk);
int dfk_mutex_free(dfk_mutex_t* mutex);
int dfk_mutex_lock(dfk_mutex_t* mutex);
int dfk_mutex_unlock(dfk_mutex_t* mutex);
int dfk_mutex_trylock(dfk_mutex_t* mutex);


typedef struct dfk_cond_t {
  struct {
    dfk_list_t waitqueue;
  } _;
  dfk_t* dfk;
} dfk_cond_t;

int dfk_cond_init(dfk_cond_t* cond, dfk_t* dfk);
int dfk_cond_free(dfk_cond_t* cond);
int dfk_cond_wait(dfk_cond_t* cond, dfk_mutex_t* mutex);
int dfk_cond_signal(dfk_cond_t* cond);
int dfk_cond_broadcast(dfk_cond_t* cond);

