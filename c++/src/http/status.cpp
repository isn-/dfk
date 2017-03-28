/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/http/status.hpp>

namespace dfk {
namespace http {

const char* reasonPhrase(Status status)
{
  return dfk_http_reason_phrase(static_cast<dfk_http_status_e>(status));
}

}} // namespace dfk::http
