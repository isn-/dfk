/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <cstdlib>
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
  dfk_t* dfk = nativeHandle();
  dfk_init(dfk);
  dfk->user.data = this;
  defaultMalloc = dfk->malloc;
  defaultFree = dfk->free;
  defaultRealloc = dfk->realloc;
  defaultLog = dfk->log;
  dfk->malloc = mallocProxy;
  dfk->free = freeProxy;
  dfk->realloc = reallocProxy;
  dfk->log = logProxy;
}

Context::~Context()
{
  dfk_free(nativeHandle());
}

void* Context::malloc(std::size_t nbytes)
{
  return defaultMalloc(nativeHandle(), nbytes);
}

void Context::free(void* p)
{
  defaultFree(nativeHandle(), p);
}

void* Context::realloc(void* p, std::size_t nbytes)
{
  return defaultRealloc(nativeHandle(), p, nbytes);
}

void Context::log(LogLevel level, const char* msg)
{
  if (defaultLog) {
    defaultLog(nativeHandle(), static_cast<int>(level), msg);
  }
}

void Context::setDefaultStackSize(std::size_t stackSize)
{
  nativeHandle()->default_stack_size = stackSize;
}

std::size_t Context::defaultStackSize() const
{
  return nativeHandle()->default_stack_size;
}

int Context::work(void (*func)(Fiber, void*), void* arg, size_t argSize)
{
  return dfk_work(nativeHandle(), (void (*)(dfk_fiber_t*, void*)) func, arg, argSize);
}

void Context::stop()
{
  DFK_ENSURE_OK(this, dfk_stop(nativeHandle()));
}

void Context::sleep(uint64_t)
{
  throw Exception(this, dfk_err_not_implemented);
}

Fiber Context::spawn(void (*func)(Fiber, void*), void* arg, size_t argSize)
{
  dfk_fiber_t* fiber = dfk_spawn(nativeHandle(), (void (*)(dfk_fiber_t*, void*)) func, arg, argSize);
  if (!fiber) {
    throw Exception(this, nativeHandle()->dfk_errno);
  }
  fiber->user.data = this;
  return Fiber(fiber);
}

} // namespace dfk
