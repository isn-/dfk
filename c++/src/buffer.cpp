/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/buffer.hpp>

namespace dfk {

Buffer::Buffer(const char* data, std::size_t size)
  : Wrapper()
{
  *nativeHandle() = (dfk_buf_t) {const_cast<char*>(data), size};
}

Buffer::Buffer(dfk_buf_t* buf)
  : Wrapper(buf)
{
}

Buffer::Buffer(const dfk_buf_t* buf)
  : Wrapper(const_cast<dfk_buf_t*>(buf))
{
}

const char* Buffer::data() const
{
  return nativeHandle()->data;
}

std::size_t Buffer::size() const
{
  return nativeHandle()->size;
}

} // namespace dfk
