/**
 * @file dfk/memmem.h
 * Contains dfk_memmem function prototype
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */
#pragma once
#include <stddef.h>
#include <dfk/config.h>

/**
 * Portable version of memmem - locate a substring
 *
 * From the memmem manpage:
 * The memmem() function finds the start of the first occurrence of the
 * substring needle of length needlelen in the memory area haystack of
 * length haystacklen.
 *
 * @returns A pointer to the beginning of the substring,
 * or NULL if the substring is not found.
 */
void* dfk_memmem(const void *haystack, size_t haystacklen,
    const void *needle, size_t needlelen);

