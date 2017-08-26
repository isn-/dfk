#include <assert.h>
#include <dfk/error.h>
#include <dfk/cond.h>
#include <dfk/internal.h>
#include <dfk/tcp_server.h>

enum {
  DFK_TCP_SERVER_INITIALIZED = 0,
  DFK_TCP_SERVER_SERVING = 1,
  DFK_TCP_SERVER_STOP_REQUESTED = 2,
  DFK_TCP_SERVER_STOP_WAIT_CONNECTIONS = 3,
  DFK_TCP_SERVER_STOPPED = 4
};

void dfk_tcp_server_init(dfk_tcp_server_t* server, dfk_t* dfk)
{
  assert(server);
  assert(dfk);
  server->dfk = dfk;
  dfk_list_hook_init(&server->_hook);
  server->_active_connections = 0;
  server->_state = DFK_TCP_SERVER_INITIALIZED;
}

void dfk_tcp_server_free(dfk_tcp_server_t* server)
{
  DFK_UNUSED(server);
  assert(server->_state == DFK_TCP_SERVER_INITIALIZED
      || server->_state == DFK_TCP_SERVER_STOPPED);
}

typedef struct listen_ud {
  dfk_tcp_server_t* server;
  dfk_tcp_handler handler;
  dfk_fiber_t* serve_fiber;
  dfk_userdata_t ud;
} listen_ud;

static void dfk_tcp_server_callback(dfk_fiber_t* fiber,
    dfk_tcp_socket_t* socket, dfk_userdata_t ud)
{
  assert(fiber);
  assert(socket);
  assert(ud.data);
  listen_ud* lud = (listen_ud*) ud.data;

  lud->server->_active_connections++;
  lud->handler(lud->server, fiber, socket, lud->ud);
  lud->server->_active_connections--;

  if (lud->server->_active_connections == 0
      && lud->server->_state == DFK_TCP_SERVER_STOP_WAIT_CONNECTIONS) {
    DFK_RESUME(lud->serve_fiber);
  }
}

int dfk_tcp_serve(dfk_tcp_server_t* server,
    const char* endpoint,
    uint16_t port,
    dfk_tcp_handler handler,
    dfk_userdata_t handler_ud,
    size_t backlog)
{
  dfk_t* dfk = server->dfk;
  dfk_list_append(&dfk->_tcp_servers, &server->_hook);
  listen_ud lud = {
    .server = server,
    .handler = handler,
    .serve_fiber = DFK_THIS_FIBER(dfk),
    .ud = handler_ud
  };

  int err = dfk_tcp_socket_init(&server->_s, server->dfk);
  if (err != dfk_err_ok) {
    return err;
  }

  server->_state = DFK_TCP_SERVER_SERVING;

  err = dfk_tcp_socket_listen(&server->_s, endpoint, port,
      dfk_tcp_server_callback, (dfk_userdata_t) {.data = &lud}, backlog);
  if (err != dfk_err_ok) {
    return err;
  }

  if (server->_active_connections) {
    server->_state = DFK_TCP_SERVER_STOP_WAIT_CONNECTIONS;
    DFK_SUSPEND(dfk);
  }

  err = dfk_tcp_socket_close(&server->_s);
  if (err != dfk_err_ok) {
    return err;
  }

  server->_state = DFK_TCP_SERVER_STOPPED;
  return dfk_err_ok;
}

int dfk_tcp_server_is_stopping(dfk_tcp_server_t* server)
{
  assert(server);
  return server->_state == DFK_TCP_SERVER_STOP_REQUESTED
    || server->_state == DFK_TCP_SERVER_STOP_WAIT_CONNECTIONS
    || server->_state == DFK_TCP_SERVER_STOPPED;
}

int dfk_tcp_server_stop(dfk_tcp_server_t* server)
{
  assert(server);
  assert(server->_state == DFK_TCP_SERVER_SERVING);
  int err = dfk_tcp_socket_shutdown(&server->_s, DFK_SHUT_RDWR);
  if (err != dfk_err_ok) {
    return err;
  }
  server->_state = DFK_TCP_SERVER_STOP_REQUESTED;
  return dfk_err_ok;
}

size_t dfk_tcp_server_sizeof(void)
{
  return sizeof(dfk_tcp_server_t);
}

