/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dfk/error.h>
#include <dfk/strtoll.h>

int dfk_strtoll(dfk_buf_t nbuf, char** endptr, int base, long long* out)
{
  /**
   * null-terminated copy of buf, for calling strtoll
   * that accept C strings only.
   */
  char ntcopy[22];
  char* ntendptr;
  assert(out);

  if (nbuf.size >= sizeof(ntcopy)) {
    return dfk_err_overflow;
  }

  memcpy(ntcopy, nbuf.data, nbuf.size);
  ntcopy[nbuf.size] = 0;
  errno = 0;
  long long res = strtoll(ntcopy, &ntendptr, base);
  if (errno == ERANGE) {
    return dfk_err_overflow;
  } else if (errno == EINVAL || ntendptr == ntcopy) {
    /*
     * EINVAL should not be returned for C99 code.
     * However, we'll keep this check here for possible
     * C89 backporting.
     */
    return dfk_err_badarg;
  }
  *out= res;
  if (endptr) {
    assert(ntendptr > ntcopy);
    *endptr = nbuf.data + (ntendptr - ntcopy);
  }
  return dfk_err_ok;
}
