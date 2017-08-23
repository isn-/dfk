/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LISENSE)
 */

#include <assert.h>
#include <unistd.h>
#include <dfk/config.h>
#include <dfk/read.h>
#include <dfk/error.h>
#include <dfk/internal.h>

ssize_t dfk__read(dfk_t* dfk, void* dfkhandle, int fd, char* buf, size_t nbytes)
{
  assert(buf);
  assert(nbytes);
  assert(dfk);
  DFK_UNUSED(dfkhandle);

  ssize_t nread = read(fd, buf, nbytes);
  DFK_DBG(dfk, "{%p} read (possibly blocking) attempt returned %lld, "
      "errno=%d",
      (void*) dfkhandle, (long long) nread, errno);
  if (nread >= 0) {
    DFK_DBG(dfk, "{%p} non-blocking read of %lld bytes", (void*) dfkhandle,
        (long long) nread);
    return nread;
  }
  if (errno != EAGAIN) {
    DFK_ERROR_SYSCALL(dfk, "read");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  int ioret = DFK_IO(dfk, fd, DFK_IO_IN);
#if DFK_DEBUG
  char strev[16];
  size_t nwritten = dfk__io_events_to_str(ioret, strev, DFK_SIZE(strev));
  DFK_DBG(dfk, "{%p} DFK_IO returned %d (%.*s)", (void*) dfkhandle, ioret,
      (int) nwritten, strev);
#endif
  if (ioret & DFK_IO_ERR) {
    DFK_ERROR_SYSCALL(dfk, "read");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  assert(ioret & DFK_IO_IN);
  nread = read(fd, buf, nbytes);
  DFK_DBG(dfk, "{%p} read returned %lld", (void*) dfkhandle, (long long) nread);
  if (nread < 0) {
    DFK_ERROR_SYSCALL(dfk, "read");
    dfk->dfk_errno = dfk_err_sys;
    return -1;
  }
  return nread;
}
