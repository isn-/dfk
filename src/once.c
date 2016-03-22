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
#include <dfk/once.h>
#include <dfk/core.h>

#define LOOP(obj) ((dfk_event_loop_t*) (obj)->_.h.loop->data)
#define CTX(obj) LOOP((obj))->_.ctx

int dfk_once_init(dfk_once_t* once, dfk_event_loop_t* loop, void (*callback)(void*), void* arg)
{
  int err;
  if (once == NULL || loop == NULL || callback == NULL) {
    return dfk_err_badarg;
  }
  if ((err = uv_prepare_init(&loop->_.loop, &once->_.h)) != 0) {
    loop->_.ctx->sys_errno = err;
    return dfk_err_sys;
  }
  once->_.h.data = once;
  once->callback = callback;
  once->arg = arg;
  return dfk_err_ok;
}


static void dfk_once_callback(uv_prepare_t* handle)
{
  dfk_once_t* once;
  int err;

  assert(handle);
  once = handle->data;
  assert(once);
  assert(once->callback);
  if ((err = uv_prepare_stop(handle)) != 0) {
    CTX(once)->sys_errno = err;
  } else {
    once->callback(once->arg);
  }
  once->callback = NULL;
}

int dfk_once_fire(dfk_once_t* once)
{
  if (once == NULL) {
    return dfk_err_badarg;
  }
  if (once->callback == NULL) {
    return dfk_err_inprog;
  }
  uv_prepare_start(&once->_.h, dfk_once_callback);
  return dfk_err_ok;
}

static void dfk_once_on_close(uv_handle_t* handle)
{
  assert(handle);
  (void) handle;
}

int dfk_once_free(dfk_once_t* once)
{
  int err;
  if (once == NULL) {
    return dfk_err_badarg;
  }
  if (once->callback) {
    /* Once object has not been fired. Force disarm.*/
    if ((err = uv_prepare_stop(&once->_.h)) != 0) {
      CTX(once)->sys_errno = err;
      return dfk_err_sys;
    }
  }
  once->arg = CTX(once)->_.current_coro;
  uv_close((uv_handle_t*) &once->_.h, dfk_once_on_close);
  return dfk_err_ok;
}

