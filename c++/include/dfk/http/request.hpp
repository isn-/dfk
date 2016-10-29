/**
 * @file dfk/http/request.hpp
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
#include <dfk.h>
#include <dfk/wrapper.hpp>
#include <dfk/core.hpp>
#include <dfk/http/method.hpp>
#include <dfk/buffer.hpp>
#include <dfk/string_map.hpp>

namespace dfk {
namespace http {

class Server;

class Request : public Wrapper<dfk_http_request_t, dfk_http_request_sizeof>
{
public:
  Request(dfk_http_request_t* request);

  const Server* server() const;
  Server* server();
  unsigned short majorVersion() const;
  unsigned short minorVersion() const;
  Method method() const;
  Buffer url() const;
  Buffer path() const;
  Buffer query() const;
  Buffer fragment() const;
  Buffer host() const;
  Buffer userAgent() const;
  Buffer contentType() const;
  uint64_t contentLength() const;
  bool keepalive() const;
  bool chunked() const;
  StringMap headers() const;
  StringMap arguments() const;

  ssize_t read(char* buf, size_t nbytes);
  ssize_t readv(IoVec* iov, size_t niov);
};

}} // namespace dfk::http
