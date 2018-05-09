/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/error.h>
#include <dfk/exception.hpp>

#define DFK_ENSURE_OK(ctx, expr) \
{\
  int err = (expr);\
  if (err != dfk_err_ok) {\
    throw dfk::Exception(ctx, err);\
  }\
}
