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

#include <dfk.h>
#include <dfk/internal.h>
#include <assert.h>

struct coro_context init;


typedef struct {
  void (*ep)(dfk_t*, void*);
  void* arg;
} dfk_coro_main_arg_t;


static void dfk_coro_main(void* arg)
{
  dfk_coro_t* coro = (dfk_coro_t*) arg;
  coro->_.ep(coro->_.dfk, coro->_.arg);
  coro->_.next = coro->_.dfk->_.termhead;
  coro->_.dfk->_.termhead = coro;
  dfk_coro_yield(coro, coro->_.dfk->_.scheduler);
}


dfk_coro_t* dfk_coro_run(dfk_t* dfk, void (*ep)(dfk_t*, void*), void* arg)
{
  if (!dfk) {
    return NULL;
  }
  if (!ep) {
    dfk->dfk_errno = dfk_err_badarg;
    return NULL;
  }
  {
    dfk_coro_t* coro = DFK_MALLOC(dfk, dfk->default_stack_size);
    char* stack_base = (char*) coro + sizeof(dfk_coro_t);
    size_t stack_size = dfk->default_stack_size - sizeof(dfk_coro_t);
    if (!coro) {
      dfk->dfk_errno = dfk_err_nomem;
      return NULL;
    }
    coro->_.ep = ep;
    coro->_.arg = arg;
    coro->_.dfk = dfk;
    coro->_.next = NULL;
    coro_create(&coro->_.ctx, dfk_coro_main, coro, stack_base, stack_size);
    DFK_INFO(dfk, "stack %p (%lu) = {%p}", (void*) stack_base, (unsigned long) stack_size, (void*) coro);
    coro->_.next = dfk->_.exechead;
    dfk->_.exechead = coro;
    return coro;
  }
}

int dfk_coro_yield(dfk_coro_t* from, dfk_coro_t* to)
{
  if (!from && !to) {
    return dfk_err_badarg;
  }
  DFK_DEBUG((from ? from : to)->_.dfk, "context switch {%p} -> {%p}",
      (void*) from, (void*) to);
  coro_transfer(from ? &from->_.ctx : &init, to ? &to->_.ctx : &init);
  return dfk_err_ok;
}

int dfk_coro_free(dfk_coro_t* coro)
{
  if (!coro) {
    return dfk_err_badarg;
  }
  DFK_FREE(coro->_.dfk, coro);
  return dfk_err_ok;
}

