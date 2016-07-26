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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dfk/internal/misc.h>


int dfk__strtoll(dfk_buf_t nbuf, char** endptr, int base, long long* out)
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


void dfk__buf_append(dfk_buf_t* to, const char* data, size_t size)
{
  assert(to);
  if (to->data) {
    assert(to->data + to->size == data);
    to->size += size;
  } else {
    to->data = (char*) data;
    to->size = size;
  }
}

