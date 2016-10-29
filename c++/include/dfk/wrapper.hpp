/**
 * @file dfk/wrapper.hpp
 *
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
