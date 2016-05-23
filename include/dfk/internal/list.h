/**
 * @file dfk/internal/list.h
 * Intrusive list data structure
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
#include <stddef.h>
#include <stdint.h>

typedef struct dfk_list_hook_t {
  struct dfk_list_hook_t* next;
  struct dfk_list_hook_t* prev;
#ifdef DFK_DEBUG
  struct dfk_list_t* list;
#endif
} dfk_list_hook_t;

typedef struct dfk_list_t {
  dfk_list_hook_t* head;
  dfk_list_hook_t* tail;
  size_t size;
} dfk_list_t;

void dfk_list_init(dfk_list_t* list);
void dfk_list_free(dfk_list_t* list);
void dfk_list_hook_init(dfk_list_hook_t* hook);
void dfk_list_hook_free(dfk_list_hook_t* hood);

void dfk_list_append(dfk_list_t* list, dfk_list_hook_t* hook);
void dfk_list_prepend(dfk_list_t* list, dfk_list_hook_t* hook);
void dfk_list_clear(dfk_list_t* list);
void dfk_list_erase(dfk_list_t* list, dfk_list_hook_t* hook);
void dfk_list_pop_front(dfk_list_t* list);
void dfk_list_pop_back(dfk_list_t* list);
size_t dfk_list_size(dfk_list_t* list);

