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


#include <dfk.h>
#include <stdio.h>

static int myperror(const char* func, dfk_context_t* ctx, int err)
{
  char* errmsg;
  if (dfk_strerr(ctx, err, &errmsg) != dfk_err_ok) {
    printf("Call to \"%s\" failed: %s\n", func, errmsg);
  }
  return 1;
}

static void connection_handler(dfk_tcp_socket_t* lsock, dfk_tcp_socket_t* sock, int listen_err)
{
  char buf[512] = {0};
  size_t nread;
  int err;

  (void) lsock;

  if (listen_err != dfk_err_ok) {
    return;
  }

  for (;;) {
    err = dfk_tcp_socket_read(sock, buf, sizeof(buf), &nread);
    if (err != dfk_err_ok) {
      break;
    }
    err = dfk_tcp_socket_write(sock, buf, nread);
    if (err != dfk_err_ok) {
      break;
    }
  }

  (void) dfk_tcp_socket_close(sock);
}

int main(void)
{
  int err;
  dfk_context_t* ctx = dfk_default_context();
  dfk_event_loop_t loop;
  dfk_tcp_socket_t socket;
  if ((err = dfk_event_loop_init(&loop, ctx)) != dfk_err_ok) {
    return myperror("dfk_event_loop_init", ctx, err);
  }
  if ((err = dfk_tcp_socket_init(&socket, &loop)) != dfk_err_ok) {
    return myperror("dfk_tcp_socket_init", ctx, err);
  }
  err = dfk_tcp_socket_start_listen(&socket, "127.0.0.1", 10000, connection_handler, 0);
  if (err != dfk_err_ok) {
    return myperror("dfk_tcp_socket_start_listen", ctx, err);
  }
  if ((err = dfk_event_loop_run(&loop)) != dfk_err_ok) {
    return myperror("dfk_event_loop_run", ctx, err);
  }
  if ((err = dfk_event_loop_join(&loop)) != dfk_err_ok) {
    return myperror("dfk_event_loop_join", ctx, err);
  }
  if ((err = dfk_tcp_socket_free(&socket)) != dfk_err_ok) {
    return myperror("dfk_tcp_socket_free", ctx, err);
  }
  return 0;
}

