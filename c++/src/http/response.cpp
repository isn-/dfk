/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
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
