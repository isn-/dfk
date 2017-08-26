/**
 * @file dfk/malloc.h
 * Memory-management functions for internal use only
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stddef.h>
#include <dfk/context.h>

void* dfk__malloc(dfk_t* dfk, size_t nbytes);
void dfk__free(dfk_t* dfk, void* p);
void* dfk__realloc(dfk_t* dfk, void* p, size_t nbytes);

