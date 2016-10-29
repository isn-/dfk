/**
 * @file dfk/http/server.hpp
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
#include <dfk/context.hpp>
#include <dfk/http/request_handler.hpp>

namespace dfk {

class Context;

} // namespace dfk

namespace dfk {
namespace http {

class Request;
class Response;

class Server : public Wrapper<dfk_http_t, dfk_http_sizeof>
{
public:
  explicit Server(Context* context);
  ~Server();
  const Context* context() const;
  Context* context();
  void serve(const char* endpoint, uint16_t port, IRequestHandler* handler);
  void stop();

private:
  static int handler(dfk_userdata_t user, dfk_http_t*, dfk_http_request_t*, dfk_http_response_t*);
};

}} // namespace dfk::http
