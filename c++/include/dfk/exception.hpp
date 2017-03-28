/**
 * @file dfk/exception.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <stdexcept>

namespace dfk {

class Context;

class Exception : public std::exception
{
public:
  Exception(Context* context, int code);

  const char* what() const throw();
  int code() const;
  Context* context() const;

private:
  Context* context_;
  int code_;
};

} // namespace dfk
