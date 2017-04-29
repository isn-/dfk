/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <string.h>
#include <dfk/error.h>

const char* dfk_strerr(dfk_t* dfk, int err)
{
  switch(err) {
    case dfk_err_ok: {
      return "dfk_err_ok(0): No error";
    }
    case dfk_err_eof: {
      return "dfk_err_eof(1): End of file (stream, iterator)";
    }
    case dfk_err_busy: {
      return "dfk_err_busy(2): Resource is already acquired";
    }
    case dfk_err_nomem: {
      return "dfk_err_nomem(3): Memory allocation function returned NULL";
    }
    case dfk_err_notfound: {
      return "dfk_err_notfound(4): Object not found";
    }
    case dfk_err_badarg: {
      return "dfk_err_badarg(5): Bad argument";
    }
    case dfk_err_sys: {
      return dfk ? strerror(dfk->sys_errno) :
        "dfk_err_sys(6): System error, dfk_t object is NULL, "
        "can not access sys_errno";
    }
    case dfk_err_inprog: {
      return "dfk_err_inprog(7): The operation is already in progress";
    }
    case dfk_err_panic: {
      return "dfk_err_panic(8): Unexpected behaviour";
    }
    case dfk_err_not_implemented: {
      return "dfk_err_not_implemented(9): Functionality is not implemented yet";
    }
    case dfk_err_overflow: {
      return "dfk_err_overflow(10): Floating point, or integer overflow error";
    }
    case dfk_err_protocol: {
      return "dfk_err_protocol(11): Protocol violation";
    }
    case dfk_err_timeout: {
      return "dfk_err_timeout(12): Timeout has expired";
    }
    default: {
      return "Unknown error";
    }
  }
  return "Unknown error";
}

