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

#include <dfk/http/response.hpp>
#include <dfk/http/server.hpp>

namespace dfk {
namespace http {

Response::Response(dfk_http_response_t* response)
  : Wrapper(response)
{
}

const Server* Response::server() const
{
  return static_cast<Server*>(nativeHandle()->http->user.data);
}

Server* Response::server()
{
  return static_cast<Server*>(nativeHandle()->http->user.data);
}

StringMap Response::headers() const
{
  return StringMap(&nativeHandle()->headers);
}

Response& Response::majorVersion(unsigned short majorVersion)
{
  nativeHandle()->major_version = majorVersion;
  return *this;
}

Response& Response::minorVersion(unsigned short minorVersion)
{
  nativeHandle()->minor_version = minorVersion;
  return *this;
}

Response& Response::status(Status status)
{
  nativeHandle()->status = static_cast<dfk_http_status_e>(status);
  return *this;
}

Response& Response::contentLength(std::size_t contentLength)
{
  nativeHandle()->content_length = contentLength;
  return *this;
}

Response& Response::chunked()
{
  nativeHandle()->chunked = 1;
  return *this;
}

Response& Response::keepalive()
{
  nativeHandle()->keepalive = 1;
  return *this;
}

ssize_t Response::write(char* buf, std::size_t nbytes)
{
  return dfk_http_response_write(nativeHandle(), buf, nbytes);
}

ssize_t Response::writev(IoVec* iov, std::size_t niov)
{
  return dfk_http_response_writev(nativeHandle(), reinterpret_cast<dfk_iovec_t*>(iov), niov);
}

Response& Response::set(const char* name, std::size_t namelen, const char* value, std::size_t valuelen)
{
  return *this;
}

}} // namespace dfk::http
