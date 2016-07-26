/**
 * @file dfk/internal/list.h
 * Intrusive list data structure
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
#include <stdint.h>
#include <dfk/config.h>

typedef struct dfk_list_hook_t {
  struct dfk_list_hook_t* next;
  struct dfk_list_hook_t* prev;
#if DFK_DEBUG
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

