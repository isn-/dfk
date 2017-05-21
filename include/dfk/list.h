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
#if DFK_LIST_MEMORY_OPTIMIZED
  /**
   * A "magic" pointer that does not point at actual memory chunk.
   *
   * The value of _p is equal to
   * (address of the element next to hook)
   *   XOR
   * (address of the element previous to hook)
   *
   * If one of both adjacent elements do not exist, it is assumed that it's
   * address is NULL.
   *
   * With one of the addresses known (next or previous), one can compute address
   * of the remaining element, therefore be able to traverse the list.
   */
  struct dfk_list_hook_t* _p;
#else
  struct dfk_list_hook_t* _next;
  struct dfk_list_hook_t* _prev;
#endif
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
#if DFK_LIST_MEMORY_OPTIMIZED
  dfk_list_hook_t* _prev;
#endif
  dfk_list_hook_t* value;
#if DFK_DEBUG
  dfk_list_t* _list;
#endif
} dfk_list_it;

typedef struct dfk_list_rit {
#if DFK_LIST_MEMORY_OPTIMIZED
  dfk_list_hook_t* _prev;
#endif
  dfk_list_hook_t* value;
#if DFK_DEBUG
  dfk_list_t* _list;
#endif
} dfk_list_rit;

void dfk_list_init(dfk_list_t* list);
void dfk_list_hook_init(dfk_list_hook_t* hook);

/**
 * Returns a pointer to the first element of the list, NULL if the list is empty
 *
 * @pre list != NULL
 */
dfk_list_hook_t* dfk_list_front(dfk_list_t* list);

/**
 * Returns a pointer to the last element of the list, NULL if the list is empty
 *
 * @pre list != NULL
 */
dfk_list_hook_t* dfk_list_back(dfk_list_t* list);

/**
 * @todo add test append_erased
 */
void dfk_list_append(dfk_list_t* list, dfk_list_hook_t* hook);

/**
 * @todo add test prepend_erased
 */
void dfk_list_prepend(dfk_list_t* list, dfk_list_hook_t* hook);

/**
 * Insert a new element before the element at the specified position
 */
void dfk_list_insert(dfk_list_t* list, dfk_list_hook_t* hook,
    dfk_list_it* position);

/**
 * Insert a new element before the element at the specified position
 */
void dfk_list_rinsert(dfk_list_t* list, dfk_list_hook_t* hook,
    dfk_list_rit* position);

/**
 * Erase all elements for the list
 */
void dfk_list_clear(dfk_list_t* list);

/**
 * Erase element pointed by `it' from the list
 *
 * @warning Value of `it' after the call is unspecified.
 */
void dfk_list_erase(dfk_list_t* list, dfk_list_it* it);

/**
 * Erase element pointed by `rit' from the list
 *
 * @warning Value of `rit' after the call is unspecified.
 */
void dfk_list_rerase(dfk_list_t* list, dfk_list_rit* rit);
void dfk_list_pop_front(dfk_list_t* list);
void dfk_list_pop_back(dfk_list_t* list);

/**
 * Moves all elements from source list to destination.
 *
 * Existing content of the destination list is discarded, source list becomes
 * empty.
 *
 * @pre src != NULL
 * @pre dst != NULL
 */
void dfk_list_move(dfk_list_t* src, dfk_list_t* dst);

/**
 * Swap content of two lists
 *
 * @note If destination list is guaranteed to be empty, use dfk_list_move.
 *
 * @pre lhs != NULL
 * @pre rhs != NULL
 */
void dfk_list_swap(dfk_list_t* lhs, dfk_list_t* rhs);

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

