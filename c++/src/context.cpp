/**
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <dfk/context.hpp>
#include "common.hpp"

namespace dfk {

static void* mallocProxy(dfk_t* dfk, size_t nbytes)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  return context->malloc(nbytes);
}

static void freeProxy(dfk_t* dfk, void* p)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  context->free(p);
}

static void* reallocProxy(dfk_t* dfk, void* p, std::size_t nbytes)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  return context->realloc(p, nbytes);
}

static void logProxy(dfk_t* dfk, int level, const char* msg)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  context->log(static_cast<LogLevel>(level), msg);
}

Context::Context()
{
  DFK_ENSURE_OK(this, dfk_init(nativeHandle()));
  nativeHandle()->user.data = this;
  nativeHandle()->malloc = mallocProxy;
  nativeHandle()->free = freeProxy;
  nativeHandle()->realloc = reallocProxy;
  nativeHandle()->log = logProxy;
}

Context::~Context()
{
  DFK_ENSURE_OK(this, dfk_free(nativeHandle()));
}

void Context::setDefaultStackSize(std::size_t stackSize)
{
  nativeHandle()->default_stack_size = stackSize;
}

std::size_t Context::defaultStackSize() const
{
  return nativeHandle()->default_stack_size;
}

void Context::work()
{
  DFK_ENSURE_OK(this, dfk_work(nativeHandle()));
}

void Context::stop()
{
  DFK_ENSURE_OK(this, dfk_stop(nativeHandle()));
}

void Context::sleep(uint64_t msec)
{
  DFK_ENSURE_OK(this, dfk_sleep(nativeHandle(), msec));
}

Coroutine Context::run(void (*func)(Coroutine, void*), void* arg, size_t argSize)
{
  dfk_coro_t* coro = dfk_run(nativeHandle(), (void (*)(dfk_coro_t*, void*)) func, arg, argSize);
  if (!coro) {
    throw Exception(this, nativeHandle()->dfk_errno);
  }
  coro->user.data = this;
  return Coroutine(coro);
}

} // namespace dfk
