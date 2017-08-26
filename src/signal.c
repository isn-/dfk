#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dfk/error.h>
#include <dfk/signal.h>
#include <dfk/internal.h>
#include <dfk/make_nonblock.h>
#include <dfk/read.h>
#include <dfk/close.h>

typedef struct waiting_fiber_t {
  dfk_list_hook_t hook;
  int wfd;
  int signum[2];
  dfk_t* dfk;
} waiting_fiber_t;

#define TO_WAITING_FIBER(expr) DFK_CONTAINER_OF((expr), waiting_fiber_t, hook)

#define DFK_SS_INFO(dfk, ...) \
do { \
  if ((dfk)->log_is_signal_safe) {\
    DFK_INFO((dfk), __VA_ARGS__); \
  } \
} while (0)

#define DFK_SS_ERROR(dfk, ...) \
do { \
  if ((dfk)->log_is_signal_safe) {\
    DFK_ERROR((dfk), __VA_ARGS__); \
  } \
} while (0)

/**
 * Static list of fibers waiting for signals definately sucks.
 * The same approach is used in libuv, however.
 *
 * If you have an idea how to implement it without static list, please
 * share your thoughts via https://github.com/ivochkin/dfk/issues/new.
 *
 * @todo Invent a better approach for per-context signal handling
 */
static dfk_list_t waiting_fibers;

static void dfk__sighandler(int signum)
{
  dfk_list_it it, end;
  dfk_list_begin(&waiting_fibers, &it);
  dfk_list_end(&waiting_fibers, &end);
  while (!dfk_list_it_equal(&it, &end)) {
    waiting_fiber_t* wf = TO_WAITING_FIBER(it.value);
    DFK_SS_INFO(wf->dfk, "{%p} Signal %d received, waiting for %d, %d",
        (void*) wf->dfk, signum, wf->signum[0], wf->signum[1]);
    if (wf->signum[0] == signum || wf->signum[1] == signum) {
      char c = 27; /* Live fast, die young, leave a beautiful corpse */
      ssize_t nwritten = write(wf->wfd, &c, sizeof(c));
      if (nwritten < 0) {
        if (wf->dfk->log_is_signal_safe) {
          DFK_SS_ERROR(wf->dfk, "Write to pipe failed");
        }
      }
    }
    dfk_list_it_next(&it);
  }
}

static int dfk__create_pipe(dfk_t* dfk, int fd[])
{
#if DFK_HAVE_PIPE2
  if (pipe2(fd, O_NONBLOCK) != 0) {
    DFK_ERROR_SYSCALL(dfk, "pipe2");
    return dfk_err_sys;
  }
  return dfk_err_ok;
#else
  if (pipe(fd) != 0) {
    DFK_ERROR_SYSCALL(dfk, "pipe");
    return dfk_err_sys;
  }
  for (int i = 0; i < 2; ++i) {
    int err = dfk__make_nonblock(dfk, fd[i]);
    if (err != dfk_err_ok) {
      dfk__close(dfk, NULL, fd[0]);
      dfk__close(dfk, NULL, fd[1]);
      return err;
    }
  }
  return dfk_err_ok;
#endif
}

static int dfk__sigwait(dfk_t* dfk, size_t nsignals, int* signum)
{
  assert(dfk);
  assert(nsignals);
  assert(signum);

  int pipefd[2];
  int err = dfk__create_pipe(dfk, pipefd);
  if (err != dfk_err_ok) {
    return err;
  }

  waiting_fiber_t wf = {
    .wfd = pipefd[1],
    .signum = {
      signum[0],
      nsignals ? signum[1] : 0
    },
    .dfk = dfk
  };
  dfk_list_hook_init(&wf.hook);
  dfk_list_append(&waiting_fibers, &wf.hook);

  struct sigaction sa = {
    .sa_handler = dfk__sighandler,
    .sa_flags = SA_RESTART
  };

  for (size_t i = 0; i < nsignals; ++i) {
    if (sigaction(signum[i], &sa, NULL) != 0) {
      DFK_ERROR_SYSCALL(dfk, "sigaction");
      dfk__close(dfk, NULL, pipefd[0]);
      dfk__close(dfk, NULL, pipefd[1]);
      return dfk_err_sys;
    }
  }

  char c;
  int finalerr = dfk_err_ok;
  ssize_t nread = dfk__read(dfk, NULL, pipefd[0], &c, 1);
  if (nread < 0) {
    finalerr = dfk_err_sys;
  }

  dfk_list_it it;
  dfk_list_it_from_value(&waiting_fibers, &wf.hook, &it);
  dfk_list_erase(&waiting_fibers, &it);

  for (int i = 0; i < 2; ++i) {
    err = dfk__close(dfk, NULL, pipefd[i]);
    if (finalerr == dfk_err_ok && err != dfk_err_ok) {
      finalerr = err;
    }
  }
  return finalerr;
}

int dfk_sigwait(dfk_t* dfk, int signum)
{
  DFK_DBG(dfk, "{%p} waiting for %d", (void*) DFK_THIS_FIBER(dfk), signum);
  return dfk__sigwait(dfk, 1, &signum);
}

int dfk_sigwait2(dfk_t* dfk, int signum1, int signum2)
{
  DFK_DBG(dfk, "{%p} waiting for %d, %d",
      (void*) DFK_THIS_FIBER(dfk), signum1, signum2);
  int signals[] = {signum1, signum2};
  return dfk__sigwait(dfk, 2, signals);
}

