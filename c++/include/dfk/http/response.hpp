/**
 * @file dfk/http/response.hpp
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
#include <dfk/http/response.h>
#include <dfk/wrapper.hpp>
#include <dfk/core.hpp>
#include <dfk/buffer.hpp>
#include <dfk/string_map.hpp>
#include <dfk/http/method.hpp>
#include <dfk/http/status.hpp>

namespace dfk {
namespace http {

class Server;

class Response : public Wrapper<dfk_http_response_t, dfk_http_response_sizeof>
{
public:
  Response(dfk_http_response_t* response);

  const Server* server() const;
  Server* server();

  StringMap headers() const;
  Response& majorVersion(unsigned short majorVersion);
  Response& minorVersion(unsigned short minorVersion);
  Response& status(Status status);
  Response& contentLength(std::size_t contentLength);
  Response& chunked();
  Response& keepalive();
  ssize_t write(char* buf, std::size_t nbytes);
  ssize_t writev(IoVec* iov, std::size_t niov);
  Response& set(const char* name, std::size_t namelen, const char* value, std::size_t valuelen);
};

}} // namespace dfk::http
