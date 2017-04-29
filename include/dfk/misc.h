/**
 * @file dfk/misc.h
 * Miscellaneous structures and functions
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>

typedef struct dfk_iovec_t {
  char* data;
  size_t size;
} dfk_iovec_t;

typedef struct dfk_buf_t {
  char* data;
  size_t size;
} dfk_buf_t;

/**
 * Append memory chunk to the buffer @p to.
 *
 * Buffers should be placed continuously in memory.
 * @private
 * @pre to->data + to->size == data
 */
void dfk_buf_append(dfk_buf_t* to, const char* data, size_t size);

/**
 * A struct to associate client data with library object.
 *
 * Compilers complain on casting function pointer to void*, so we
 * use a separate union member when function pointer is required.
 */
typedef union dfk_userdata_t {
  void* data;
  void (*func)(void);
} dfk_userdata_t;

/**
 * Returns size of the dfk_buf_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_buf_sizeof(void);

/**
 * Returns size of the dfk_iovec_t structure.
 *
 * @see dfk_sizeof
 */
size_t dfk_iovec_sizeof(void);

