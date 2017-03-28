/**
 * @file dfk/middleware/fileserver.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk.h>
#include <dfk/wrapper.hpp>
#include <dfk/context.hpp>
#include <dfk/buffer.hpp>
#include <dfk/http/request_handler.hpp>

namespace dfk {
namespace fileserver {

class Server : public Wrapper<dfk_fileserver_t, dfk_fileserver_sizeof>
    , public http::IRequestHandler
{
public:
  explicit Server(Context* context, const Buffer& basepath);
  void setAutoindex(bool enabled);
  void setIOBufferSize(std::size_t size);

  int handle(http::Server*, http::Request&, http::Response&);
};

}} // namespace dfk::fileserver
