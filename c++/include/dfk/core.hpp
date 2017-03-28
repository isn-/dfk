/**
 * @file dfk/core.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk.h>

namespace dfk {

enum LogLevel
{
  ERROR = dfk_log_error,
  WARNING = dfk_log_warning,
  INFO = dfk_log_info,
  DEBUG = dfk_log_debug
};

typedef dfk_iovec_t IoVec;

} // namespace dfk
