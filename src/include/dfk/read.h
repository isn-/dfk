/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <sys/types.h>
#include <dfk/context.h>

ssize_t dfk__read(dfk_t* dfk, void* dfkhandle, int sock, char* buf, size_t nbytes);

