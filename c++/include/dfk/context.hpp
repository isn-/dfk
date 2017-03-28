/**
 * @file dfk/context.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <cstddef>
#include <dfk.h>
#include <dfk/core.hpp>
#include <dfk/wrapper.hpp>
#include <dfk/coroutine.hpp>

namespace dfk {

class Context : public Wrapper<dfk_t, dfk_sizeof>
{
public:
  Context();
  ~Context();

private:
  Context(const Context&);
  Context& operator=(const Context&);

public:
  virtual void* malloc(std::size_t nbytes);
  virtual void free(void* p);
  virtual void* realloc(void* p, std::size_t nbytes);
  virtual void log(LogLevel level, const char* msg);

  void setDefaultStackSize(std::size_t stackSize);
  std::size_t defaultStackSize() const;
  void work();
  void stop();
  void sleep(uint64_t msec);
  /// @todo Refactor it. Need template method similar to boost::bind
  Coroutine run(void (*func)(Coroutine, void*), void* arg, size_t argSize);

private:
  void* (*defaultMalloc)(dfk_t*, size_t);
  void (*defaultFree)(dfk_t*, void*);
  void* (*defaultRealloc)(dfk_t*, void*, size_t);
  void (*defaultLog)(dfk_t*, int, const char*);
};

} // namespace dfk
