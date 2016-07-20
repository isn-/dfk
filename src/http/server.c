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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dfk/sync.h>
#include <dfk/internal.h>
#include <dfk/http/server.h>
#include <dfk/http/protocol.h>


typedef struct dfk__event_list_item_t {
  dfk_list_hook_t hook;
  dfk_event_t event;
} dfk__event_list_item_t;


static void dfk__http_connection(dfk_coro_t* coro, dfk_tcp_socket_t* sock, void* p)
{
  dfk_http_t* http = (dfk_http_t*) p;

  /*
   * dfk_list_hook_t + dfk_mutex_t to be placed into dfk_http_t._connections
   * list. When dfk_http_stop is called, it waits for all current requests
   * to be processed gracefully by locking each mutex. The mutex is inserted
   * into the dfk_http_t._connections list in a locked state and is released
   * only when connection is closed indicating that no wait is required for
   * this connection.
   */
  dfk__event_list_item_t item;
  dfk_list_hook_init(&item.hook);
  DFK_CALL_RVOID(http->dfk, dfk_event_init(&item.event, http->dfk));
  dfk_list_append(&http->_connections, &item.hook);

  dfk__http(coro, sock, http);

  dfk_list_erase(&http->_connections, &item.hook);
  dfk_list_hook_free(&item.hook);
  DFK_CALL_RVOID(http->dfk, dfk_event_signal(&item.event));
  DFK_CALL_RVOID(http->dfk, dfk_event_free(&item.event));
}


int dfk_http_init(dfk_http_t* http, dfk_t* dfk)
{
  if (!http || !dfk) {
    return dfk_err_badarg;
  }
  DFK_DBG(dfk, "{%p}", (void*) http);
  http->dfk = dfk;
  http->keepalive_requests = 100;
  dfk_list_init(&http->_connections);
  dfk_event_init(&http->_stopped, dfk);
  return dfk_tcp_socket_init(&http->_listensock, dfk);
}


int dfk_http_stop(dfk_http_t* http)
{
  if (!http) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  dfk_list_erase(&http->dfk->_http_servers, &http->_hook);
  DFK_CALL(http->dfk, dfk_tcp_socket_close(&http->_listensock));
  DFK_DBG(http->dfk, "{%p} no longer accepts new connections, "
          "wait for running requests to terminate", (void*) http);
  DFK_CALL(http->dfk, dfk_event_wait(&http->_stopped));
  return dfk_err_ok;
}


int dfk_http_free(dfk_http_t* http)
{
  if (!http) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p}", (void*) http);
  DFK_CALL(http->dfk, dfk_event_free(&http->_stopped));
  dfk_list_free(&http->_connections);
  return dfk_tcp_socket_free(&http->_listensock);
}


int dfk_http_serve(dfk_http_t* http,
    const char* endpoint,
    uint16_t port,
    dfk_http_handler handler)
{
  if (!http || !endpoint || !handler || !port) {
    return dfk_err_badarg;
  }
  DFK_DBG(http->dfk, "{%p} serving at %s:%u", (void*) http, endpoint, port);
  http->_handler = handler;
  dfk_list_append(&http->dfk->_http_servers, &http->_hook);
  DFK_CALL(http->dfk, dfk_tcp_socket_listen(&http->_listensock, endpoint, port,
                                            dfk__http_connection, http, 0));

  dfk_list_hook_t* i = http->_connections.head;
  while (i) {
    dfk__event_list_item_t* it = (dfk__event_list_item_t*) i;
    DFK_DBG(http->dfk, "{%p} waiting for {%p} to terminate", (void*) http, (void*) i);
    DFK_CALL(http->dfk, dfk_event_wait(&it->event));
    i = http->_connections.head;
  }

  /* Wake up coroutine that called dfk_stop */
  DFK_CALL(http->dfk, dfk_event_signal(&http->_stopped));
  return dfk_err_ok;
}

