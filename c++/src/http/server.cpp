/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/internal.h>
#include <dfk/context.hpp>
#include <dfk/http/request.hpp>
#include <dfk/http/response.hpp>
#include <dfk/http/server.hpp>
#include "../common.hpp"

namespace dfk {
namespace http {

Server::Server(Context* context)
{
  dfk_http_init(nativeHandle(), context->nativeHandle());
  nativeHandle()->user.data = this;
}

Server::~Server()
{
  dfk_http_free(nativeHandle());
}

const Context* Server::context() const
{
  return static_cast<Context*>(nativeHandle()->dfk->user.data);
}

Context* Server::context()
{
  return static_cast<Context*>(nativeHandle()->dfk->user.data);
}

void Server::serve(const char* endpoint, uint16_t port, std::size_t backlog,
    IRequestHandler* handler)
{
  dfk_userdata_t user = (dfk_userdata_t) {handler};
  DFK_ENSURE_OK(context(), dfk_http_serve(nativeHandle(), endpoint, port, backlog, Server::handler, user));
}

void Server::stop()
{
  DFK_ENSURE_OK(context(), dfk_http_stop(nativeHandle()));
}

int Server::handler(dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp, dfk_userdata_t user)
{
  // This method is called from the C code.
  // Therefore, no exception should pass boundaries of this method.
  Server* self = static_cast<Server*>(http->user.data);
  IRequestHandler* handler = static_cast<IRequestHandler*>(user.data);
  Request request(req);
  Response response(resp);
  try {
    return handler->handle(self, request, response);
  } catch(const std::exception& ex) {
    DFK_ERROR(self->context()->nativeHandle(),
        "{%p} exception during processing request {%p}: \"%s\"",
        (void*) http, (void*) req, ex.what());
  } catch(...) {
    DFK_ERROR(self->context()->nativeHandle(),
        "{%p} unhandled exception during processing request {%p}",
        (void*) http, (void*) req);
  }
  /// @todo A new error code is needed here
  return dfk_err_not_implemented;
}

}} // namespace dfk::http
