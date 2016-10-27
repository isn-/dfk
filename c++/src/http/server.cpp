/**
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
  DFK_ENSURE_OK(context, dfk_http_init(nativeHandle(), context->nativeHandle()));
  nativeHandle()->user.data = this;
}

Server::~Server()
{
  DFK_ENSURE_OK(context(), dfk_http_free(nativeHandle()));
}

const Context* Server::context() const
{
  return static_cast<Context*>(nativeHandle()->dfk->user.data);
}

Context* Server::context()
{
  return static_cast<Context*>(nativeHandle()->dfk->user.data);
}

void Server::serve(const char* endpoint, uint16_t port)
{
  dfk_userdata_t ud = {NULL};
  DFK_ENSURE_OK(context(), dfk_http_serve(nativeHandle(), endpoint, port, handler, ud));
}

void Server::stop()
{
  DFK_ENSURE_OK(context(), dfk_http_stop(nativeHandle()));
}

int Server::handler(dfk_userdata_t, dfk_http_t* http, dfk_http_request_t* req, dfk_http_response_t* resp)
{
  // This method is called from the C code.
  // Therefore, no exception should pass boundaries of this method.
  Server* self = static_cast<Server*>(http->user.data);
  Request request(req);
  Response response(resp);
  try {
    return self->handleRequest(request, response);
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
