/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LISENSE)
 */

#include <assert.h>
#include <unistd.h>
#include <sys/select.h>
#include <dfk/config.h>
#include <dfk/eventloop.h>
#include <dfk/internal.h>
#include <dfk/error.h>

#define TO_FDLIST_ELEMENT(expr) \
  DFK_CONTAINER_OF((expr), dfk_fdlist_element_t, hook)

#if DFK_DEBUG
static int dfk__eventloop_fd_in_set(dfk_eventloop_t* loop, int fd)
{
  dfk_list_it it, end;
  dfk_list_begin(&loop->fds, &it);
  dfk_list_end(&loop->fds, &end);
  while (!dfk_list_it_equal(&it, &end)) {
    if (TO_FDLIST_ELEMENT(it.value)->fd == fd) {
      return 1;
    }
  }
  return 0;
}
#endif

int dfk__eventloop_init(dfk_eventloop_t* loop, dfk_t* dfk)
{
  assert(loop);
  assert(dfk);
  loop->dfk = dfk;
  dfk_list_init(&loop->fds);
  return 0;
}

int dfk__eventloop_free(dfk_eventloop_t* loop)
{
  assert(loop);
  DFK_UNUSED(loop);
  DFK_IF_DEBUG(loop = DFK_PDEADBEEF);
  return 0;
}

void dfk__eventloop_main(dfk_fiber_t* fiber, void* arg)
{
  dfk_eventloop_t* loop = (dfk_eventloop_t*) arg;
  dfk_t* dfk = fiber->dfk;
  assert(loop);
  assert(loop->dfk == fiber->dfk);
  fd_set readfds;
  fd_set writefds;
  fd_set exceptfds;
  while (1) {
    if (dfk_list_empty(&loop->fds)) {
      DFK_DBG(dfk, "{%p} no fd needs IO, suspend", (void*) loop);
      DFK_SUSPEND(dfk);
    }
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    int maxfd = 0;
    {
      dfk_list_it it, end;
      dfk_list_begin(&loop->fds, &it);
      dfk_list_end(&loop->fds, &end);
      while (!dfk_list_it_equal(&it, &end)) {
        dfk_fdlist_element_t* e = TO_FDLIST_ELEMENT(it.value);
        if (e->events & DFK_IO_IN) {
          FD_SET(e->fd, &readfds);
        }
        if (e->events & DFK_IO_OUT) {
          FD_SET(e->fd, &writefds);
        }
        FD_SET(e->fd, &exceptfds);
        maxfd = DFK_MAX(maxfd, e->fd);
        dfk_list_it_next(&it);
      }
    }
    DFK_DBG(dfk, "{%p} call select with max fd = %d", (void*) loop, maxfd);
    int nfd = select(maxfd + 1, &readfds, &writefds, &exceptfds, NULL);
    if (nfd == -1) {
      DFK_ERROR_SYSCALL(dfk, "select");
      break;
    }
    if (nfd == 0) {
      DFK_DBG(dfk, "{%p} select returned 0, susped IO", (void*) loop);
      DFK_SUSPEND(dfk);
      continue;
    }
    {
      dfk_list_it it, end;
      dfk_list_begin(&loop->fds, &it);
      dfk_list_end(&loop->fds, &end);
      while (!dfk_list_it_equal(&it, &end)) {
        dfk_fdlist_element_t* e = TO_FDLIST_ELEMENT(it.value);
        int r = FD_ISSET(e->fd, &readfds),
            w = FD_ISSET(e->fd, &writefds),
            ex = FD_ISSET(e->fd, &exceptfds);
        if (r || w || ex) {
          e->events = 0;
          if (r) {
            e->events |= DFK_IO_IN;
          }
          if (w) {
            e->events |= DFK_IO_OUT;
          }
          if (ex) {
            e->events |= DFK_IO_ERR;
          }
#if DFK_DEBUG
          char strev[16];
          size_t nwritten = dfk__io_events_to_str(e->events, strev,
              DFK_SIZE(strev));
          DFK_DBG(dfk, "{%p} fd %d, events %d (%.*s), yieldback fiber %p",
              (void*) loop, e->fd, e->events, (int) nwritten, strev,
              (void*) e->yieldback);
#endif
          /* See dfk_list_erase documentation to get the purpose of `itcopy' */
          dfk_list_it itcopy = it;
          dfk_list_it_next(&it);
          dfk_list_erase(&loop->fds, &itcopy);
          DFK_IORESUME(e->yieldback);
        } else {
          DFK_DBG(dfk, "{%p} no events received for fd %d",
              (void*) loop, e->fd);
          dfk_list_it_next(&it);
        }
      }
    }
  }
}

int dfk__io(dfk_eventloop_t* loop, int socket, int events)
{
  assert(loop);
  assert(socket);
  dfk_t* dfk = loop->dfk;
  assert(dfk);
  DFK_IF_DEBUG(assert(!dfk__eventloop_fd_in_set(loop, socket)));
  dfk_fdlist_element_t e = {
    .fd = socket,
    .events = events,
    .yieldback = DFK_THIS_FIBER(dfk),
  };
  dfk_list_append(&loop->fds, &e.hook);
#if DFK_DEBUG
  char strev[16];
  size_t nwritten = dfk__io_events_to_str(events, strev, DFK_SIZE(strev));
  DFK_DBG(dfk, "{%p} add fd %d to select fd set, events %d (%.*s)",
    (void*) loop, socket, events, (int) nwritten, strev);
#endif
  DFK_IOSUSPEND(dfk);
  return e.events;
}

