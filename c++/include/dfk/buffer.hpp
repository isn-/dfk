/**
 * @file dfk/buffer.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
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
