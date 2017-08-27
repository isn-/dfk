/**
 * @file close.c
 *
 * Contains dfk__close implementation
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LISENSE)
 */

#include <assert.h>
#include <unistd.h>
#include <dfk/error.h>
#include <dfk/close.h>
#include <dfk/internal.h>

int dfk__close(dfk_t* dfk, void* dfkhandle, int sock)
{
  DFK_UNUSED(dfkhandle);
  DFK_INFO(dfk, "{%p} closing", dfkhandle);
  if (close(sock) == -1) {
    DFK_ERROR_SYSCALL(dfk, "close");
    return dfk_err_sys;
  }
  return dfk_err_ok;
}

