/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */


#include <dfk/fiber.hpp>
#include "common.hpp"

namespace dfk {

Fiber::Fiber(dfk_fiber_t* fiber)
  : Wrapper(fiber)
{
}

void Fiber::setName(const char* name)
{
  dfk_fiber_name(nativeHandle(), "%s", name);
}

Context* Fiber::context()
{
  return static_cast<Context*>(nativeHandle()->dfk->user.data);
}

void Fiber::yield(Fiber* to)
{
  dfk_yield(this->nativeHandle(), to->nativeHandle());
}

} // namespace dfk
