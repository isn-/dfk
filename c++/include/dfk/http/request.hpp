/**
 * @file dfk/http/request.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/http/request.h>
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
