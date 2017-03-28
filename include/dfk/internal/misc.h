/**
 * @file dfk/internal/misc.h
 * Miscellaneous functions
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/core.h>

/**
 * strtoll for pascal strings
 * @private
 * @warning Does not preserve errno upon success
 */
int dfk_strtoll(dfk_buf_t nbuf, char** endptr, int base, long long* out);


/**
 * Append memory chunk to the buffer @p to.
 *
 * Buffers should be placed continuously in memory.
 * @private
 * @pre to->data + to->size == data
 */
void dfk_buf_append(dfk_buf_t* to, const char* data, size_t size);

