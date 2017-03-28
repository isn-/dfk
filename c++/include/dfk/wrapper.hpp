/**
 * @file dfk/wrapper.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <assert.h>
#include <algorithm>

namespace dfk {

template<class T, size_t Size(void)>
class Wrapper
{
public:
  Wrapper()
    : owns_(true)
  {
    objBuffer_ = new char[std::max(sizeof(T), Size())];
  }

  explicit Wrapper(T* obj)
    : owns_(false)
  {
    obj_ = obj;
  }

  ~Wrapper()
  {
    if (owns_) {
      delete[] objBuffer_;
    }
  }

  Wrapper(const Wrapper& other)
  {
    *this = other;
  }

  Wrapper& operator=(const Wrapper& other)
  {
    assert(!other.owns() && "Object is not copyable");
    owns_ = other.owns_;
    obj_ = other.obj_;
    return *this;
  }

  T* nativeHandle()
  {
    return obj_;
  }

  const T* nativeHandle() const
  {
    return obj_;
  }

  bool owns() const
  {
    return owns_;
  }

private:
  bool owns_;
  union {
    char* objBuffer_;
    T* obj_;
  };
};

} // namespace dfk
