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

static void connection_handler(dfk_tcp_socket_t* socket)
{
  char buf[512] = {0};
  size_t nread, nwritten;
  int err;

  for (;;) {
    err = dfk_tcp_socket_read(socket, buf, sizeof(buf), &nread);
    if (err != dfk_err_ok) {
      break;
    }
    err = dfk_tcp_socket_write(socket, buf, nread, &nwritten);
    if (err != dfk_err_ok || nwritten != nread) {
      break;
    }
  }

  (void) dfk_tcp_socket_close(socket);
}

int main(void)
{
  dfk_context_t* ctx = dfk_default_context();
  dfk_event_loop_t loop;
  dfk_tcp_socket_t socket;
  dfk_event_loop_init(&loop, ctx);
  dfk_tcp_socket_init(&socket, &loop);
  dfk_tcp_socket_listen(&socket, "localhost", 10000, connection_handler, 0);
  dfk_event_loop_run(&loop);
  dfk_event_loop_join(&loop);
  return 0;
}

