/**
 * @file dfk/coroutine.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk.h>
#include <dfk/wrapper.hpp>

namespace dfk {

class Context;

class Coroutine : public Wrapper<dfk_coro_t, dfk_coro_sizeof>
{
  friend class Context;

private:
  Coroutine(dfk_coro_t* coro);

public:
  void setName(const char* name);
  Context* context();
  void yield(Coroutine* to);
};

} // namespace dfk
