/**
 * @copyright
 * Copyright (c) 2015-2016 Stanislav Ivochkin
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

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <dfk/config.h>
#include <dfk/middleware/fileserver.h>
#include <dfk/internal.h>

typedef struct dirent_list_item_t {
  dfk_list_hook_t hook;
  struct dirent de;
} dirent_list_item_t;

static const char autoindex_header_1[] =
  "<html>\n"
  "  <head>\n"
  "    <title>Directory listing for ";

static const char autoindex_header_2[] =
  "</title>\n"
  "  </head>\n"
  "  <body>\n"
  "    <h2>Directory listing for ";

static const char autoindex_header_3[] =
  "</h2>\n"
  "    <hr>\n"
  "    <ul>\n";

static const char autoindex_footer[] =
  "    </ul>\n"
  "    <hr>\n"
  "  </body>\n"
  "</html>\n";

static int ignore_file(struct dirent* de)
{
  if (!(de->d_type & (DT_DIR | DT_REG))) {
    return 0;
  }
#if __APPLE__
  return (de->d_name[0] == '.') && (de->d_namlen == 1 || (de->d_name[1] == '.' && de->d_namlen == 2));
#endif
#if __linux__
  return (de->d_name[0] == '.') && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0'));
#endif
}

int dfk_fileserver_init(dfk_fileserver_t* fs, dfk_t* dfk, const char* basepath, ssize_t basepathlen)
{
  assert(fs);
  assert(dfk);
  assert(basepath);
  fs->_basepath = DFK_MALLOC(dfk, basepathlen);
  if (fs->_basepath == NULL) {
    return dfk_err_nomem;
  }
  memcpy(fs->_basepath, basepath, basepathlen);
  fs->_basepathlen = basepathlen;
  fs->dfk = dfk;
  fs->io_buf_size = DFK_FILESERVER_BUFFER_SIZE;
  fs->autoindex |= 1;
  return dfk_err_ok;
}

int dfk_fileserver_free(dfk_fileserver_t* fs)
{
  DFK_FREE(fs->dfk, fs->_basepath);
  return dfk_err_ok;
}

size_t dfk_fileserver_sizeof(void)
{
  return sizeof(dfk_fileserver_t);
}

int dfk_fileserver_handler(dfk_userdata_t ud, dfk_http_t* http,
    dfk_http_request_t* request, dfk_http_response_t* response)
{
  DFK_UNUSED(http);
  dfk_fileserver_t* fs = (dfk_fileserver_t*) ud.data;
  if (request->method == DFK_HTTP_GET) {
    size_t fullpathlen = fs->_basepathlen + request->path.size + 1;
    char* fullpath = dfk_arena_alloc(request->_request_arena, fullpathlen);
    if (fullpath == NULL) {
      DFK_ERROR(fs->dfk, "out of memory");
      response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
      return dfk_err_ok;
    }
    memcpy(fullpath, fs->_basepath, fs->_basepathlen);
    memcpy(fullpath + fs->_basepathlen, request->path.data, request->path.size);
    fullpath[fullpathlen - 1] = '\0';
    DFK_DBG(fs->dfk, "get stat of '%s'", fullpath);
    struct stat statinfo;
    memset(&statinfo, 0, sizeof(statinfo));
    if (stat(fullpath, &statinfo) == -1) {
      if (errno == ENOENT || errno == ENOTDIR) {
        DFK_DBG(fs->dfk, "file not found");
        response->status = DFK_HTTP_NOT_FOUND;
      } else {
        response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
      }
      return dfk_err_ok;
    }
    if (statinfo.st_mode & S_IFDIR) {
      if (!fs->autoindex) {
        response->status = DFK_HTTP_NOT_FOUND;
        return dfk_err_ok;
      } else {
        DIR* dp = opendir(fullpath);
        dfk_list_t filelist;
        dfk_list_init(&filelist);
        if (dp == NULL) {
          response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
          return dfk_err_ok;
        }
        struct dirent* res;
        do {
          dirent_list_item_t* dei = dfk_arena_alloc(request->_request_arena, sizeof(dirent_list_item_t));
          if (!dei) {
            closedir(dp);
            response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
            return dfk_err_ok;
          }
          dfk_list_hook_init(&dei->hook);
          do {
            if (readdir_r(dp, &dei->de, &res) != 0) {
              closedir(dp);
              response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
              return dfk_err_ok;
            }
            if (res && !ignore_file(res)) {
              assert(res == &dei->de);
              dfk_list_append(&filelist, &dei->hook);
              break;
            }
          } while (res);
        } while (res);
        closedir(dp);
        size_t niov = 6 + 6 * filelist.size;
        int add_separator_after_path = request->path.data[request->path.size - 1] != '/';
        if (add_separator_after_path) {
          niov += filelist.size;
        }
        dfk_iovec_t* iov = dfk_arena_alloc(request->_request_arena, niov * sizeof(dfk_iovec_t));
        if (!iov) {
          response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
          return dfk_err_ok;
        }
        size_t iiov = 0;
        iov[iiov++] = (dfk_iovec_t) {(char*) autoindex_header_1, DFK_SIZE(autoindex_header_1)};
        iov[iiov++] = (dfk_iovec_t) {request->path.data, request->path.size};
        iov[iiov++] = (dfk_iovec_t) {(char*) autoindex_header_2, DFK_SIZE(autoindex_header_2)};
        iov[iiov++] = (dfk_iovec_t) {request->path.data, request->path.size};
        iov[iiov++] = (dfk_iovec_t) {(char*) autoindex_header_3, DFK_SIZE(autoindex_header_3)};
        for (dfk_list_hook_t* it = filelist.head; it; it = it->next) {
          dirent_list_item_t* i = (dirent_list_item_t*) it;
          iov[iiov++] = (dfk_iovec_t) {"      <li><a href=\"", 19};
          iov[iiov++] = (dfk_iovec_t) {request->path.data, request->path.size};
          if (add_separator_after_path) {
            iov[iiov++] = (dfk_iovec_t) {"/", 1};
          }
#if __APPLE__
          const size_t namelen = i->de.d_namlen;
#else
          const size_t namelen = strnlen(i->de.d_name, sizeof(i->de.d_name));
#endif
          iov[iiov++] = (dfk_iovec_t) {i->de.d_name, namelen};
          iov[iiov++] = (dfk_iovec_t) {"\">", 2};
          iov[iiov++] = (dfk_iovec_t) {i->de.d_name, namelen};
          iov[iiov++] = (dfk_iovec_t) {"</a></li>\n", 10};
        }
        iov[iiov++] = (dfk_iovec_t) {(char*) autoindex_footer, DFK_SIZE(autoindex_footer)};
        size_t content_length = 0;
        for (size_t i = 0; i < niov; ++i) {
          content_length += iov[i].size;
        }
        response->content_length = content_length;
        dfk_http_response_writev(response, iov, niov);
        dfk_list_free(&filelist);
        response->status = DFK_HTTP_OK;
        return dfk_err_ok;
      }
    } else {
      size_t towrite = statinfo.st_size;
      response->content_length = towrite;
      FILE* f = fopen(fullpath, "r");
      if (f == NULL) {
        DFK_ERROR(fs->dfk, "can not open file '%s'", fullpath);
        response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
        return dfk_err_ok;
      }
      char* buffer = DFK_MALLOC(fs->dfk, fs->io_buf_size);
      if (buffer == NULL) {
        DFK_ERROR(fs->dfk, "out of memory");
        fclose(f);
        response->status = DFK_HTTP_INTERNAL_SERVER_ERROR;
        return dfk_err_ok;
      }
      while (towrite) {
        DFK_DBG(fs->dfk, "need to write %llu bytes", (unsigned long long) towrite);
        size_t toread = DFK_MIN(fs->io_buf_size, towrite);
        size_t nread = fread(buffer, 1, toread, f);
        if (nread < toread) {
          fs->dfk->sys_errno = errno;
          DFK_ERROR(fs->dfk, "error reading file: %s", dfk_strerr(fs->dfk, dfk_err_sys));
          fclose(f);
          DFK_FREE(fs->dfk, buffer);
          return dfk_err_sys;
        }
        ssize_t nwritten = dfk_http_response_write(response, buffer, nread);
        if (nwritten < 0) {
          fclose(f);
          DFK_FREE(fs->dfk, buffer);
          return fs->dfk->dfk_errno;
        }
        assert(nwritten <= (ssize_t) towrite);
        towrite -= nwritten;
      }
      fclose(f);
      DFK_FREE(fs->dfk, buffer);
    }
  } else {
    response->status = DFK_HTTP_METHOD_NOT_ALLOWED;
  }
  return dfk_err_ok;
}

