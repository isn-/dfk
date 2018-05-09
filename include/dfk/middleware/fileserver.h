/**
 * @file dfk/middleware/fileserver.h
 * Static files server
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/config.h>
#include <dfk/context.h>
#include <dfk/http.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

