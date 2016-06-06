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

#include <stdlib.h>
#include <stdio.h>
#include <dfk.h>
#include <dfk/internal.h>


typedef struct args_t {
  int argc;
  char** argv;
} args_t;


static void echo(dfk_t* dfk, dfk_http_req_t* req, dfk_http_resp_t* resp)
{
  DFK_UNUSED(dfk);
  DFK_UNUSED(req);
  resp->code = 200;
}


static void dfk_main(dfk_coro_t* coro, void* p)
{
  dfk_http_t srv;
  args_t* args = (args_t*) p;
  DFK_CALL_RVOID(coro->dfk, dfk_http_init(&srv, coro->dfk));
  DFK_CALL_RVOID(coro->dfk, dfk_http_serve(&srv, args->argv[1], atoi(args->argv[2]), echo));
}


int main(int argc, char** argv)
{
  dfk_t dfk;
  args_t args;
  if (argc != 3) {
    fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
    return 1;
  }
  args.argc = argc;
  args.argv = argv;
  dfk_init(&dfk);
  (void) dfk_run(&dfk, dfk_main, &args, sizeof(args));
  DFK_CALL(&dfk, dfk_work(&dfk));
  return dfk_free(&dfk);
}
