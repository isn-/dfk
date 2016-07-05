/**
 * @copyright
 * Copyright (c) 2016, Stanislav Ivochkin. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dfk/internal/misc.h>


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
  } else if (ntendptr == ntcopy || errno == EINVAL) {
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


