/**
 * @file dfk/urlencoding.h
 * URL decoding and encoding functions
 *
 * @see RFC-3986 https://tools.ietf.org/html/rfc3986
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns size of the buffer needed to store the result of percent-decoding.
 */
size_t dfk_urldecode_hint(const char* buf, size_t size);

/**
 * Decode percent-encoded buffer.
 *
 * If out is NULL in-place decoding is performed.
 *
 * @return Number of bytes consumed
 */
size_t dfk_urldecode(char* buf, size_t size, char* out, size_t* nwritten);

/**
 * Returns size of the buffer needed to store the result of percent-encoding.
 */
size_t dfk_urlencode_hint(const char* buf, size_t size);

/**
 * Percent encode buffer.
 */
size_t dfk_urlencode(const char* buf, size_t size, char* out);

#ifdef __cplusplus
}
#endif

