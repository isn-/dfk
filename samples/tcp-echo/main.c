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

