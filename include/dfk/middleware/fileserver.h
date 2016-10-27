/**
 * @file dfk/middleware/fileserver.h
 * Static files server
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
#include <dfk/config.h>
#include <dfk/core.h>
#include <dfk/http.h>

#if DFK_FILESERVER

typedef struct dfk_fileserver_t {
  /** @privatesection */
  /**
   * NULL-terminated base path
   */
  char* _basepath;
  size_t _basepathlen;

  /** @publicsection */
  dfk_t* dfk;
  size_t io_buf_size;
  /** Generate index for directories */
  int autoindex : 1;
} dfk_fileserver_t;

/**
 * @param basepath Directory to serve files from
 * @param basepathlen Size of the basepath, in bytes, excluding zero termination byte, if presented
 */
int dfk_fileserver_init(dfk_fileserver_t* fs, dfk_t* dfk, const char* basepath, ssize_t basepathlen);
int dfk_fileserver_free(dfk_fileserver_t* fs);

size_t dfk_fileserver_sizeof(void);

int dfk_fileserver_handler(dfk_userdata_t ud, dfk_http_t* http,
    dfk_http_request_t* request, dfk_http_response_t* response);

#endif

