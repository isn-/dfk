/**
 * @file dfk/http/response.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk.h>
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
