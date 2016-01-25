/**
 * @copyright
 * Copyright (c) 2015, 2016, Stanislav Ivochkin. All Rights Reserved.
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

#include <dfk/core.h>
#include <dfk/buf.h>
#include "common.h"

int dfk_buf_init(dfk_buf_t* buf, dfk_context_t* ctx, size_t nbytes)
{
  if (buf == NULL) {
    return dfk_err_badarg;
  }
  buf->_.context = ctx;
  buf->size = nbytes;
  buf->data = DFK_MALLOC(ctx, nbytes);
  if (buf->data == NULL) {
    return dfk_err_nomem;
  }
  return dfk_err_ok;
}

int dfk_buf_free(dfk_buf_t* buf)
{
  if (buf == NULL) {
    return dfk_err_badarg;
  }
  DFK_FREE(buf->_.context, buf->data);
  return dfk_err_ok;
}

