/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */


#include <dfk/coroutine.hpp>
#include "common.hpp"

namespace dfk {

Coroutine::Coroutine(dfk_coro_t* coro)
  : Wrapper(coro)
{
}

void Coroutine::setName(const char* name)
{
  DFK_ENSURE_OK(context(), dfk_coro_name(nativeHandle(), "%s", name));
}

Context* Coroutine::context()
{
  return static_cast<Context*>(nativeHandle()->dfk->user.data);
}

void Coroutine::yield(Coroutine* to)
{
  DFK_ENSURE_OK(context(), dfk_yield(this->nativeHandle(), to->nativeHandle()));
}

} // namespace dfk
