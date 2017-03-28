/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
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
