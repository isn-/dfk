/**
 * @file dfk/fiber.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/fiber.h>
#include <dfk/wrapper.hpp>

namespace dfk {

class Context;

class Fiber : public Wrapper<dfk_fiber_t, dfk_fiber_sizeof>
{
  friend class Context;

private:
  Fiber(dfk_fiber_t* fiber);

public:
  void setName(const char* name);
  Context* context();
  void yield(Fiber* to);
};

} // namespace dfk
