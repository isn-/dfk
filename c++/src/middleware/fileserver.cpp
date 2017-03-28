/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/middleware/fileserver.hpp>
#include <dfk/http/server.hpp>
#include "../common.hpp"

namespace dfk {
namespace fileserver {

Server::Server(Context* context, const Buffer& basepath)
{
  DFK_ENSURE_OK(context,
      dfk_fileserver_init(
        nativeHandle(),
        context->nativeHandle(),
        basepath.data(), basepath.size()));
}

void Server::setAutoindex(bool enabled)
{
  nativeHandle()->autoindex = enabled;
}

void Server::setIOBufferSize(std::size_t size)
{
  nativeHandle()->io_buf_size = size;
}

int Server::handle(http::Server* server, http::Request& request, http::Response& response)
{
  dfk_userdata_t user = (dfk_userdata_t) {nativeHandle()};
  return dfk_fileserver_handler(user,
      server->nativeHandle(),
      request.nativeHandle(),
      response.nativeHandle());
}

}} // namespace dfk::fileserver
