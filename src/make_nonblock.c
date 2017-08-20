#include <fcntl.h>
#include <unistd.h>
#include <dfk/make_nonblock.h>
#include <dfk/error.h>
#include <dfk/internal.h>

int dfk__make_nonblock(dfk_t* dfk, int s)
{
  int flags = fcntl(s, F_GETFL, 0);
  if (flags == -1) {
    DFK_ERROR_SYSCALL(dfk, "fcntl");
    close(s);
    return dfk_err_sys;
  }
  flags = fcntl(s, F_SETFL, flags | O_NONBLOCK);
  if (flags == -1) {
    DFK_ERROR_SYSCALL(dfk, "fnctl");
    close(s);
    return dfk_err_sys;
  }
  return dfk_err_ok;
}

