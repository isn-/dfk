/**
 * @file dfk/http/handler.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/http/request.hpp>
#include <dfk/http/response.hpp>

namespace dfk {
namespace http {

class Server;

class IRequestHandler
{
public:
  virtual int handle(Server*, Request&, Response&) = 0;
  virtual ~IRequestHandler();
};

}} // namespace dfk::http
