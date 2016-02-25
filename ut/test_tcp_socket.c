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

#include <unistd.h>
#include "ut.h"
#include <dfk/context.h>
#include <dfk/tcp_socket.h>
#include <dfk/event_loop.h>
#include "naivetp/naivetp.h"


static void on_connect(dfk_tcp_socket_t* sock)
{
  *((int*) sock->userdata) = 1;
  dfk_tcp_socket_close(sock);
}

TEST(tcp_socket, connect_disconnect)
{
  dfk_context_t* ctx = dfk_default_context();
  naivetp_server_t* ntp_server = naivetp_server_start(ctx, 10020);
  int connected = 0;
  dfk_tcp_socket_t sock;
  dfk_coro_t coro;
  dfk_event_loop_t loop;

  ASSERT(ntp_server);
  ASSERT_OK(dfk_event_loop_init(&loop, ctx));
  ASSERT_OK(dfk_tcp_socket_init(&sock, &loop));
  ASSERT_OK(dfk_tcp_socket_start_connect(&sock, "127.0.0.1", 10020, on_connect, &coro, 0));
  sock.userdata = &connected;
  ASSERT_OK(dfk_event_loop_run(&loop));
  ASSERT_OK(dfk_event_loop_join(&loop));
  EXPECT(connected == 1);
  ASSERT_OK(dfk_tcp_socket_free(&sock));
  ASSERT_OK(dfk_event_loop_free(&loop));
  naivetp_server_stop(ntp_server);
}

