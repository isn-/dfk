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

#include <dfk/http/constants.h>


const char* dfk__http_reason_phrase(dfk_http_status_e status)
{
  switch (status) {
    case DFK_HTTP_CONTINUE: return "Continue";
    case DFK_HTTP_SWITCHING_PROTOCOLS: return "Switching Protocols";
    case DFK_HTTP_PROCESSING: return "Processing";
    case DFK_HTTP_OK: return "OK";
    case DFK_HTTP_CREATED: return "Created";
    case DFK_HTTP_ACCEPTED: return "Accepted";
    case DFK_HTTP_NON_AUTHORITATIVE_INFORMATION: return "Non Authoritative information";
    case DFK_HTTP_NO_CONTENT: return "No Content";
    case DFK_HTTP_RESET_CONTENT: return "Reset Content";
    case DFK_HTTP_PARTIAL_CONTENT: return "Partial Content";
    case DFK_HTTP_MULTI_STATUS: return "Multi Status";
    case DFK_HTTP_ALREADY_REPORTED: return "Already Reported";
    case DFK_HTTP_IM_USED: return "IM Used";
    case DFK_HTTP_MULTIPLE_CHOICES: return "Multiple Choices";
    case DFK_HTTP_MOVED_PERMANENTLY: return "Moved Permanently";
    case DFK_HTTP_FOUND: return "Found";
    case DFK_HTTP_SEE_OTHER: return "See Other";
    case DFK_HTTP_NOT_MODIFIED: return "Not Modified";
    case DFK_HTTP_USE_PROXY: return "Use Proxy";
    case DFK_HTTP_SWITCH_PROXY: return "Switch Proxy";
    case DFK_HTTP_TEMPORARY_REDIRECT: return "Temporary Redirect";
    case DFK_HTTP_PERMANENT_REDIRECT: return "Permanent Redirect";
    case DFK_HTTP_BAD_REQUEST: return "Bad Request";
    case DFK_HTTP_UNAUTHORIZED: return "Unauthorized";
    case DFK_HTTP_PAYMENT_REQUIRED: return "Payment Required";
    case DFK_HTTP_FORBIDDEN: return "Forbidden";
    case DFK_HTTP_NOT_FOUND: return "Not Found";
    case DFK_HTTP_METHOD_NOT_ALLOWED: return "Method Not Allowed";
    case DFK_HTTP_NOT_ACCEPTABLE: return "Not Acceptable";
    case DFK_HTTP_PROXY_AUTHENTICATION_REQUIRED: return "Proxy Authentication Required";
    case DFK_HTTP_REQUEST_TIMEOUT: return "Request Timeout";
    case DFK_HTTP_CONFLICT: return "Conflict";
    case DFK_HTTP_GONE: return "Gone";
    case DFK_HTTP_LENGTH_REQUIRED: return "Length Required";
    case DFK_HTTP_PRECONDITION_FAILED: return "Precondition Failed";
    case DFK_HTTP_PAYLOAD_TOO_LARGE: return "Payload Too Large";
    case DFK_HTTP_URI_TOO_LONG: return "Uri Too Long";
    case DFK_HTTP_UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
    case DFK_HTTP_RANGE_NOT_SATISFIABLE: return "Range Not Satisfiable";
    case DFK_HTTP_EXPECTATION_FAILED: return "Expectation Failed";
    case DFK_HTTP_I_AM_A_TEAPOT: return "I'm a teapot";
    case DFK_HTTP_MISDIRECTED_REQUEST: return "Misdirected Request";
    case DFK_HTTP_UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
    case DFK_HTTP_LOCKED: return "Locked";
    case DFK_HTTP_FAILED_DEPENDENCY: return "Failed Dependency";
    case DFK_HTTP_UPGRADE_REQUIRED: return "Upgrade Required";
    case DFK_HTTP_PRECONDITION_REQUIRED: return "Precondition Required";
    case DFK_HTTP_TOO_MANY_REQUESTS: return "Too Many Requests";
    case DFK_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE: return "Request Header Fields Too Large";
    case DFK_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS: return "Unavailable For Legal Reasons";
    case DFK_HTTP_INTERNAL_SERVER_ERROR: return "Internal Server Error";
    case DFK_HTTP_NOT_IMPLEMENTED: return "Not Implemented";
    case DFK_HTTP_BAD_GATEWAY: return "Bad Gateway";
    case DFK_HTTP_SERVICE_UNAVAILABLE: return "Service Unavailable";
    case DFK_HTTP_GATEWAY_TIMEOUT: return "Gateway Timeout";
    case DFK_HTTP_HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
    case DFK_HTTP_VARIANT_ALSO_NEGOTIATES: return "Variant Also Negotiates";
    case DFK_HTTP_INSUFFICIENT_STORAGE: return "Insufficient Storage";
    case DFK_HTTP_LOOP_DETECTED: return "Loop Detected";
    case DFK_HTTP_NOT_EXTENDED: return "Not Extended";
    case DFK_HTTP_NETWORK_AUTHENTICATION_REQUIRED: return "Network Authentication Required";
    default: return "Unknown";
  }
}

