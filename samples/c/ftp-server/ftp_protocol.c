#include <stdio.h>
#include <string.h>
#include <dfk/error.h>
#include <dfk/memmem.h>
#include <dfk/tcp_server.h>


void ftp_connection(dfk_tcp_server_t* server, dfk_fiber_t* fiber,
    dfk_tcp_socket_t* sock, dfk_userdata_t arg);

static char greeting[] = "220 Welcome\r\n";
static char password_prompt[] = "331 Password please\r\n";
static char login_success[] = "230 Greetings\r\n";
static char server_description[] = "215 UNIX Type: L8\r\n";
static char server_features[] = "211-Extensions supported:\r\n211 END\r\n";

typedef enum {
  CMD_USER = 0,
  CMD_PASS,
  CMD_SYST,
  CMD_FEAT,
  CMD_QUIT,
} command_type;

typedef struct command_t {
  command_type type;
  dfk_buf_t arg;
} command_t;

typedef int (*command_handler)(dfk_tcp_socket_t* sock, command_t*);

/*
 * Read and parse incoming command
 *
 * @param [out] eoc End of command
 * @param [out] eob End of buffer
 */
static int read_command(dfk_tcp_socket_t* sock, char* buf, size_t size,
    size_t capacity, command_t* cmd, char** eoc, char** eob)
{
  char* readbuf = buf + size;
  char* end = buf + capacity;
  /* Read from socket until a full string that ends with \r\n is obtained */
  do {
    char* pos = dfk_memmem(buf, readbuf - buf, "\r\n", 2);
    if (pos) {
      *eoc = pos;
      *eob = readbuf;
      break;
    }
    if (readbuf == end) {
      return dfk_err_eof;
    }
    ssize_t nread = dfk_tcp_socket_read(sock, readbuf, end - readbuf);
    if (nread < 0) {
      return sock->dfk->dfk_errno;
    }
    readbuf += nread;
  } while (1);

  /* Parse incoming command */
  if (!strncmp(buf, "PASS", 4)) {
    cmd->type = CMD_PASS;
    cmd->arg = (dfk_buf_t) {
      .data = buf + 5,
      .size = *eoc - (buf + 5)
    };
  } else if (!strncmp(buf, "USER", 4)) {
    cmd->type = CMD_USER;
    cmd->arg = (dfk_buf_t) {
      .data = buf + 5,
      .size = *eoc - (buf + 5)
    };
  } else if (!strncmp(buf, "SYST", 4)) {
    cmd->type = CMD_SYST;
  } else if (!strncmp(buf, "FEAT", 4)) {
    cmd->type = CMD_FEAT;
  } else if (!strncmp(buf, "QUIT", 4)) {
    cmd->type = CMD_QUIT;
  } else {
    return dfk_err_not_implemented;
  }
  return dfk_err_ok;
}

/*
 * Write exactly entire buffer to socket.
 *
 * If partial write occurs, the function retries to write the remainder.
 */
static int sendall(dfk_tcp_socket_t* sock, char* buf, size_t size)
{
  while (size) {
    ssize_t nwritten = dfk_tcp_socket_write(sock, buf, size);
    if (nwritten <= 0) {
      return sock->dfk->dfk_errno;
    }
    size -= nwritten;
  }
  return dfk_err_ok;
}

static int handle_user(dfk_tcp_socket_t* sock, command_t* cmd)
{
  printf("Login attempt by %.*s\n", (int) cmd->arg.size, cmd->arg.data);
  int err = sendall(sock, password_prompt, sizeof(password_prompt));
  if (err != dfk_err_ok) {
    return err;
  }
  return dfk_err_ok;
}

static int handle_pass(dfk_tcp_socket_t* sock, command_t* cmd)
{
  printf("Provided password of length %d\n", (int) cmd->arg.size);
  int err = sendall(sock, login_success, sizeof(login_success));
  if (err != dfk_err_ok) {
    return err;
  }
  return dfk_err_ok;
}

static int handle_syst(dfk_tcp_socket_t* sock, command_t* cmd)
{
  (void) cmd;
  int err = sendall(sock, server_description, sizeof(server_description));
  if (err != dfk_err_ok) {
    return err;
  }
  return dfk_err_ok;
}

static int handle_feat(dfk_tcp_socket_t* sock, command_t* cmd)
{
  (void) cmd;
  int err = sendall(sock, server_features, sizeof(server_features));
  if (err != dfk_err_ok) {
    return err;
  }
  return dfk_err_ok;
}

static int handle_quit(dfk_tcp_socket_t* sock, command_t* cmd)
{
  (void) cmd;
  (void) sock;
  printf("Client sent QUIT command, closing connection\n");
  return dfk_err_eof;
}

static command_handler handlers[] = {
  handle_user, /* CMD_USER */
  handle_pass, /* CMD_PASS */
  handle_syst, /* CMD_SYST */
  handle_feat, /* CMD_FEAT */
  handle_quit, /* CMD_QUIT*/
};

void ftp_connection(dfk_tcp_server_t* server, dfk_fiber_t* fiber,
    dfk_tcp_socket_t* sock, dfk_userdata_t arg)
{
  (void) server;
  (void) fiber;
  (void) arg;

  /* Send greeting */
  int err = sendall(sock, greeting, sizeof(greeting));
  if (err != dfk_err_ok) {
    (void) dfk_tcp_socket_close(sock);
    return;
  }

  /*
   * Buffer that stores current command,
   * flushed each time read_command is called
   */
  char cmdbuf[1024] = {0};
  size_t size = 0;
  while (1) {
    command_t cmd;
    char* eoc;
    char* eob;
    err = read_command(sock, cmdbuf, size, sizeof(cmdbuf), &cmd, &eoc, &eob);
    if (err != dfk_err_ok) {
      break;
    }
    if (handlers[cmd.type]) {
      err = handlers[cmd.type](sock, &cmd);
      if (err != dfk_err_ok) {
        break;
      }
    }
    eoc += 2; /* skip \r\n */
    if (eob > eoc) {
      size = eob - eoc;
      memcpy(cmdbuf, eoc, size);
    }
  }
  (void) dfk_tcp_socket_close(sock);
}

