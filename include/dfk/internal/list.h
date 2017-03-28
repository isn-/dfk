/**
 * @file dfk/internal/list.h
 * Intrusive list data structure
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
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

