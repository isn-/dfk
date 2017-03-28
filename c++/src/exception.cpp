/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk.h>
#include <dfk/exception.hpp>
#include <dfk/context.hpp>

namespace dfk {

Exception::Exception(Context* context, int code)
  : context_(context)
  , code_(code)
{
}

const char* Exception::what() const throw()
{
  return dfk_strerr(context_ ? context_->nativeHandle() : NULL, code_);
}

int Exception::code() const
{
  return code_;
}

class Context* Exception::context() const
{
  return context_;
}

} // namespace dfk
