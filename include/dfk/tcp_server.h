/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/list.h>
#include <dfk/mutex.h>
#include <dfk/tcp_socket.h>

typedef struct dfk_tcp_server_t {
  dfk_t* dfk;
  /**
   * @privatesection
   */
  dfk_list_hook_t _hook;
  dfk_tcp_socket_t _s;

  dfk_atomic_size_t _active_connections;

  /**
   * State, can be one of:
   *
   * 0 - initialized, not serving yet
   * 1 - serving
   * 2 - stopping
   * 3 - stopped
   */
  sig_atomic_t _state;
} dfk_tcp_server_t;

void dfk_tcp_server_init(dfk_tcp_server_t* server, dfk_t* dfk);
void dfk_tcp_server_free(dfk_tcp_server_t* server);

typedef void (*dfk_tcp_handler)(dfk_tcp_server_t*,
    dfk_fiber_t*, dfk_tcp_socket_t*, dfk_userdata_t ud);

int dfk_tcp_serve(dfk_tcp_server_t* server,
    const char* endpoint,
    uint16_t port,
    dfk_tcp_handler handler,
    dfk_userdata_t handler_ud,
    size_t backlog);

int dfk_tcp_server_is_stopping(dfk_tcp_server_t* server);
int dfk_tcp_server_stop(dfk_tcp_server_t* server);

size_t dfk_tcp_server_sizeof(void);
