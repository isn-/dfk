/**
 * @file dfk/http/method.hpp
 *
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

#pragma once
#include <dfk.h>

namespace dfk {
namespace http {

enum Method
{
  DELETE = DFK_HTTP_DELETE,
  GET = DFK_HTTP_GET,
  HEAD = DFK_HTTP_HEAD,
  POST = DFK_HTTP_POST,
  PUT = DFK_HTTP_PUT,
  CONNECT = DFK_HTTP_CONNECT,
  OPTIONS = DFK_HTTP_OPTIONS,
  TRACE = DFK_HTTP_TRACE,
  COPY = DFK_HTTP_COPY,
  LOCK = DFK_HTTP_LOCK,
  MKCOL = DFK_HTTP_MKCOL,
  MOVE = DFK_HTTP_MOVE,
  PROPFIND = DFK_HTTP_PROPFIND,
  PROPPATCH = DFK_HTTP_PROPPATCH,
  SEARCH = DFK_HTTP_SEARCH,
  UNLOCK = DFK_HTTP_UNLOCK,
  BIND = DFK_HTTP_BIND,
  REBIND = DFK_HTTP_REBIND,
  UNBIND = DFK_HTTP_UNBIND,
  ACL = DFK_HTTP_ACL,
  REPORT = DFK_HTTP_REPORT,
  MKACTIVITY = DFK_HTTP_MKACTIVITY,
  CHECKOUT = DFK_HTTP_CHECKOUT,
  MERGE = DFK_HTTP_MERGE,
  MSEARCH = DFK_HTTP_MSEARCH,
  NOTIFY = DFK_HTTP_NOTIFY,
  SUBSCRIBE = DFK_HTTP_SUBSCRIBE,
  UNSUBSCRIBE = DFK_HTTP_UNSUBSCRIBE,
  PATCH = DFK_HTTP_PATCH,
  PURGE = DFK_HTTP_PURGE,
  MKCALENDAR = DFK_HTTP_MKCALENDAR,
  LINK = DFK_HTTP_LINK,
  UNLINK = DFK_HTTP_UNLINK
};

}} // namespace dfk::http
