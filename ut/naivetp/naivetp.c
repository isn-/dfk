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

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include <naivetp/naivetp.h>


typedef struct _naivetp_server_t {
  int sock;
  pthread_t thread;
  dfk_context_t* ctx;
  struct client_t* clients;
} _naivetp_server_t;


typedef struct client_t {
  int sock;
  pthread_t thread;
  dfk_context_t* ctx;
  struct client_t* next;
  _naivetp_server_t* srv;
} client_t;


static void* conn_thread(void* arg)
{
  client_t* c = (client_t*) arg;
  DFK_INFO(c->ctx, "{%p} spawned", (void*) c);
  while (1) {
    ssize_t nread = 0;
    char buf[512];
    if ((nread = read(c->sock, buf, sizeof(buf))) > 0) {
      ssize_t nwritten = 0;
      DFK_INFO(c->ctx, "{%p} bytes read %ld", (void*) c, (long) nread);
      nwritten = write(c->sock, buf, nread);
      DFK_INFO(c->ctx, "{%p} bytes written %ld", (void*) c, (long) nwritten);
    } else {
      break;
    }
  }
  DFK_INFO(c->ctx, "{%p} close", (void*) c);
  (void) close(c->sock);
  return c;
}


static void* main_thread(void* arg)
{
  _naivetp_server_t* s = (_naivetp_server_t*) arg;
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  int csock;

  DFK_INFO(s->ctx, "{%p} spawned", (void*) s);

  while ((csock = accept(s->sock, (struct sockaddr*) &addr, &addrlen)) > 0) {
    client_t* newclient = NULL;
    DFK_INFO(s->ctx, "{%p} new connection %s:%d", (void*) s, inet_ntoa(addr.sin_addr), addr.sin_port);
    newclient = (client_t*) calloc(1, sizeof(client_t));
    newclient->sock = csock;
    newclient->ctx = s->ctx;
    newclient->next = s->clients;
    newclient->srv = s;
    s->clients = newclient;
    if (pthread_create(&newclient->thread, NULL, conn_thread, newclient) != 0) {
      DFK_ERROR(s->ctx, "can not create thread");
      break;
    }
  }
  DFK_INFO(s->ctx, "{%p} close", (void*) s);
  (void) close(s->sock);
  return NULL;
}


naivetp_server_t* naivetp_server_start(dfk_context_t* ctx, uint16_t port)
{
  naivetp_server_t* s = NULL;
  struct sockaddr_in addr;
  int reuse = 1;

  if (ctx == NULL) {
    return NULL;
  }

  s = (void*) ctx->malloc(NULL, sizeof(_naivetp_server_t));
  if (s == NULL) {
    return NULL;
  }

  s->ctx = ctx;
  s->sock = socket(AF_INET, SOCK_STREAM, 0);
  if (s->sock < 0) {
    goto cleanup;
  }
  s->clients = NULL;

  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s->sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    DFK_ERROR(ctx, "can not bind socket: %s", strerror(errno));
    goto cleanup;
  }

  if (setsockopt(s->sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof(reuse)) < 0) {
    DFK_ERROR(ctx, "enable SO_REUSEADDR failed: %s", strerror(errno));
    goto cleanup;
  }

  DFK_INFO(ctx, "listen to port %d", port);
  if (listen(s->sock, -1) < 0) {
    DFK_ERROR(ctx, "can not listen: %s", strerror(errno));
    goto cleanup;
  }

  if (pthread_create(&s->thread, NULL, &main_thread, s) != 0) {
    goto cleanup;
  }

  return s;

cleanup:
  DFK_ERROR(ctx, "failed");
  if (s) {
    ctx->free(NULL, s);
    s = NULL;
  }
  return s;
}

void naivetp_server_stop(naivetp_server_t* srv)
{
  void* retval;
  client_t* client;

  if (srv == NULL) {
    return;
  }

  client = srv->clients;
  (void) shutdown(srv->sock, SHUT_RDWR);
  pthread_join(srv->thread, &retval);
  while (client) {
    (void) shutdown(client->sock, SHUT_RDWR);
    pthread_join(client->thread, &retval);
    client = client->next;
    srv->ctx->free(NULL, retval);
  }
  srv->ctx->free(NULL, srv);
}

