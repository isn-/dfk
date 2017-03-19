/**
 * @file dfk/buffer.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
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
#include <cstddef>
#include <dfk.h>
#include <dfk/wrapper.hpp>

namespace dfk {

class Buffer : public Wrapper<dfk_buf_t, dfk_buf_sizeof>
{
public:
  Buffer(const char* data, std::size_t size);
  explicit Buffer(dfk_buf_t* buf);
  explicit Buffer(const dfk_buf_t* buf);
  const char* data() const;
  std::size_t size() const;

  /// Cast Buffer to any other data-holder, i.e. std::string,
  /// boost::string_ref, std::string_view, etc.
  template<class T>
  T as()
  {
    return T(data(), size());
  }
};

} // namespace dfk
