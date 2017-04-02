/**
 * @file dfk/list.h
 * Intrusive doubly linked list data structure
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <dfk/config.h>

/**
 * Intrusive doubly linked list
 */
typedef struct dfk_list_hook_t {
  struct dfk_list_hook_t* _next;
  struct dfk_list_hook_t* _prev;
#if DFK_DEBUG
  struct dfk_list_t* _list;
#endif
} dfk_list_hook_t;

typedef struct dfk_list_t {
  /**
   * @private
   */
  dfk_list_hook_t* _head;
  /**
   * @private
   */
  dfk_list_hook_t* _tail;
#if DFK_LIST_CONSTANT_TIME_SIZE
  /**
   * @private
   */
  size_t _size;
#endif
} dfk_list_t;

typedef struct dfk_list_it {
  dfk_list_hook_t* value;
#if DFK_DEBUG
  dfk_list_t* _list;
#endif
} dfk_list_it;

typedef struct dfk_list_rit {
  dfk_list_hook_t* value;
#if DFK_DEBUG
  dfk_list_t* _list;
#endif
} dfk_list_rit;

void dfk_list_init(dfk_list_t* list);
void dfk_list_hook_init(dfk_list_hook_t* hook);

void dfk_list_append(dfk_list_t* list, dfk_list_hook_t* hook);
void dfk_list_prepend(dfk_list_t* list, dfk_list_hook_t* hook);
void dfk_list_insert(dfk_list_t* list, dfk_list_hook_t* hook,
    dfk_list_it* it);
void dfk_list_rinsert(dfk_list_t* list, dfk_list_hook_t* hook,
    dfk_list_rit* rit);
void dfk_list_clear(dfk_list_t* list);
void dfk_list_erase(dfk_list_t* list, dfk_list_it* it);
void dfk_list_rerase(dfk_list_t* list, dfk_list_rit* rit);
void dfk_list_pop_front(dfk_list_t* list);
void dfk_list_pop_back(dfk_list_t* list);

/**
 * Returns number of elements in the list
 *
 * @note To check list for emptyness, use dfk_list_empty() instead of
 * comparing dfk_list_size() to zero. When DFK_LIST_CONSTANT_TIME_SIZE
 * is disabled, dfk_list_empty() is more efficient than dfk_list_size().
 *
 * @pre list != NULL
 * @param[in] list A valid pointer to dfk_list_t structure
 * @returns Number of elements in the list, 0 if the list is empty
 */
size_t dfk_list_size(dfk_list_t* list);

/**
 * Returns non-zero if list has at least 1 element
 *
 * @pre list != NULL
 * @param[in] list A valid pointer to dfk_list_t structure
 * @returns non-zero if list is empty, 0 otherwise
 */
int dfk_list_empty(dfk_list_t* list);
size_t dfk_list_sizeof(void);

void dfk_list_begin(dfk_list_t* list, dfk_list_it* it);
void dfk_list_end(dfk_list_t* list, dfk_list_it* it);
void dfk_list_it_next(dfk_list_it* it);
int dfk_list_it_equal(dfk_list_it* lhs, dfk_list_it* rhs);
size_t dfk_list_it_sizeof(void);

void dfk_list_rbegin(dfk_list_t* list, dfk_list_rit* rit);
void dfk_list_rend(dfk_list_t* list, dfk_list_rit* rit);
void dfk_list_rit_next(dfk_list_rit* rit);
int dfk_list_rit_equal(dfk_list_rit* lhs, dfk_list_rit* rhs);
size_t dfk_list_rit_sizeof(void);

