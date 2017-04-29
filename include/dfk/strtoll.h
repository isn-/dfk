/**
 * @file dfk/strtoll.h
 * Contains dfk_strtoll function prototype
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */
#pragma once
#include <dfk/misc.h>

/**
 * strtoll, but for pascal strings
 *
 * Converts string to long long.
 * @warning Does not preserve errno upon success
 */
int dfk_strtoll(dfk_buf_t nbuf, char** endptr, int base, long long* out);

