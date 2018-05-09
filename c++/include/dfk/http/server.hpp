/**
 * @file dfk/http/server.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/http/server.h>
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
  void serve(const char* endpoint, uint16_t port, std::size_t backlog,
      IRequestHandler* handler);
  void stop();

private:
  static int handler(dfk_http_t*, dfk_http_request_t*, dfk_http_response_t*, dfk_userdata_t);
};

}} // namespace dfk::http
