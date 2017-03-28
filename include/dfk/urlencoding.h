/**
 * @file dfk/urlencoding.h
 * URL decoding and encoding functions
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>

/**
 * Decode url-encoded buffer buf of size size.
 *
 * If out is NULL in-place decoding is performed.
 *
 * @return Number of bytes consumed
 */
size_t dfk_urldecode(char* buf, size_t size, char* out, size_t* byteswritten);

size_t dfk_urlencode(const char* buf, size_t size, char* out);

