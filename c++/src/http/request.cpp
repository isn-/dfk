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

#include <dfk/http/request.hpp>
#include <dfk/http/server.hpp>

namespace dfk {
namespace http {

Request::Request(dfk_http_request_t* request)
  : Wrapper(request)
{
}

const Server* Request::server() const
{
  return static_cast<Server*>(nativeHandle()->http->user.data);
}

Server* Request::server()
{
  return static_cast<Server*>(nativeHandle()->http->user.data);
}

unsigned short Request::majorVersion() const
{
  return nativeHandle()->major_version;
}

unsigned short Request::minorVersion() const
{
  return nativeHandle()->minor_version;
}

Method Request::method() const
{
  return static_cast<Method>(nativeHandle()->method);
}

Buffer Request::url() const
{
  return Buffer(&nativeHandle()->url);
}

Buffer Request::path() const
{
  return Buffer(&nativeHandle()->path);
}

Buffer Request::query() const
{
  return Buffer(&nativeHandle()->query);
}

Buffer Request::fragment() const
{
  return Buffer(&nativeHandle()->fragment);
}

Buffer Request::host() const
{
  return Buffer(&nativeHandle()->host);
}

Buffer Request::userAgent() const
{
  return Buffer(&nativeHandle()->user_agent);
}

Buffer Request::contentType() const
{
  return Buffer(&nativeHandle()->content_type);
}

uint64_t Request::contentLength() const
{
  return nativeHandle()->content_length;
}

bool Request::keepalive() const
{
  return nativeHandle()->keepalive;
}

bool Request::chunked() const
{
  return nativeHandle()->chunked;
}

StringMap Request::headers() const
{
  return StringMap(&nativeHandle()->headers);
}

StringMap Request::arguments() const
{
  return StringMap(&nativeHandle()->arguments);
}

ssize_t Request::read(char* buf, size_t nbytes)
{
  return dfk_http_request_read(nativeHandle(), buf, nbytes);
}

ssize_t Request::readv(IoVec* iov, size_t niov)
{
  return dfk_http_request_readv(nativeHandle(), reinterpret_cast<dfk_iovec_t*>(iov), niov);
}

}} // namespace dfk::http
