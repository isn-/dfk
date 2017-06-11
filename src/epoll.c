/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LISENSE)
 */

#include <unistd.h>
#include <sys/epoll.h>
#include <dfk/eventloop.h>
#include <dfk/internal.h>
#include <dfk/error.h>

typedef struct dfk_epoll_arg_t {
  dfk_fiber_t* yieldback;
  int events;
  int fd;
} dfk_epoll_arg_t;

int dfk__eventloop_init(dfk_eventloop_t* epoll, dfk_t* dfk)
{
  assert(epoll);
  assert(dfk);
  int fd = epoll_create(1);
  if (fd == -1) {
    DFK_ERROR_SYSCALL(dfk, "epoll_create(2)");
    return dfk_err_sys;
  }
  epoll->dfk = dfk;
  epoll->fd = fd;
  DFK_DBG(dfk, "{%p} epoll_create() = %d", (void*) epoll, fd);
  return dfk_err_ok;
}

int dfk__eventloop_free(dfk_eventloop_t* epoll)
{
  assert(epoll);
  assert(epoll->fd);
  int ret = close(epoll->fd);
  if (ret < 0) {
    DFK_ERROR_SYSCALL(epoll->dfk, "close(2)");
    return dfk_err_sys;
  }
  return dfk_err_ok;
}

void dfk__eventloop_main(dfk_fiber_t* fiber, void* arg)
{
  assert(fiber);
  dfk_t* dfk = fiber->dfk;
  dfk_eventloop_t* epoll = (dfk_eventloop_t*) arg;
  assert(dfk);
  assert(epoll);
  assert(epoll->dfk == dfk);
  assert(epoll->fd);

  while (1) {
    struct epoll_event fds[64];
    int nfd = epoll_wait(epoll->fd, fds, DFK_SIZE(fds), 1000);
    if (nfd == -1) {
      DFK_ERROR_SYSCALL(dfk, "epoll_wait(2)");
      break;
    }
    DFK_DBG(dfk, "%d fds ready", nfd);
    if (nfd == 0) {
      DFK_SUSPEND(dfk);
      continue;
    }
    for (int i = 0; i < nfd; ++i) {
      dfk_epoll_arg_t* arg = (dfk_epoll_arg_t*) fds[i].data.ptr;
      arg->events = fds[i].events;
#if DFK_DEBUG
      char strev[16];
      size_t nwritten = dfk__io_events_to_str(arg->events, strev,
          DFK_SIZE(strev));
      DFK_DBG(dfk, "{%p} fd %d, events %d (%.*s), yieldback fiber %p",
          (void*) epoll, arg->fd, arg->events, (int) nwritten, strev,
          (void*) arg->yieldback);
#endif
      DFK_IORESUME(arg->yieldback);
      int ret = epoll_ctl(epoll->fd, EPOLL_CTL_DEL, arg->fd, NULL);
      if (ret == -1) {
        DFK_ERROR_SYSCALL(dfk, "epoll_ctl(2)");
        break;
      }
    }
    if (nfd) {
      DFK_SUSPEND(dfk);
    }
  }
}

int dfk__io(dfk_eventloop_t* epoll, int socket, int events)
{
  assert(epoll);
  dfk_t* dfk = epoll->dfk;
  assert(dfk);
  dfk_epoll_arg_t arg = {
    .yieldback = DFK_THIS_FIBER(dfk),
    .events = 0,
    .fd = socket,
  };
  struct epoll_event event;
  event.data.ptr = &arg;
  event.events = events;
  int ret = epoll_ctl(epoll->fd, EPOLL_CTL_ADD, socket, &event);
  if (ret == -1) {
    DFK_ERROR_SYSCALL(dfk, "epoll_ctl(2)");
    return dfk_err_sys;
  }
#if DFK_DEBUG
  char strev[16];
  size_t nwritten = dfk__io_events_to_str(events, strev, DFK_SIZE(strev));
  DFK_DBG(dfk, "{%p} add fd %d to epoll, events %d (%.*s)", (void*) epoll,
      socket, events, (int) nwritten, strev);
#endif
  DFK_IOSUSPEND(dfk);
  return arg.events;
}

