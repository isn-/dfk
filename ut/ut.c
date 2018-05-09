/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <Python.h>
#include <dfk.h>
#include <dfk/internal.h>
#include <ut.h>

typedef struct ut_case_t {
  const char* group;
  const char* name;
  void (*func)(void);
  struct ut_case_t* next;
} ut_case_t;

typedef struct {
  ut_case_t* head;
} ut_case_list_t;

static ut_case_list_t ut_all;
static ut_case_t* ut_running;

static ut_oom_policy_e ut_oom_policy;
static uint64_t ut_alloc_counter;
static size_t ut_oom_arg;

void ut_register_test_case(const char* group, const char* name, void (*func)(void))
{
  ut_case_t* new_case = (ut_case_t*) calloc(1, sizeof(ut_case_t));
  new_case->group = group;
  new_case->name = name;
  new_case->func = func;
  new_case->next = ut_all.head;
  ut_all.head = new_case;
}

void ut_error(const char* file, const int line, const char* message)
{
  printf("        %s.%s FAIL (%s:%d): unit test assertion failed: \"%s\"\n",
      ut_running->group, ut_running->name, file, line, message);
  exit(1);
}

void ut_skip(const char* group, const char* name, const char* reason)
{
  printf("        %s.%s SKIP - %s\n", group, name, reason);
}

static void* ut_malloc(dfk_t* dfk, size_t size)
{
  DFK_UNUSED(dfk);
  ++ut_alloc_counter;
  switch (ut_oom_policy) {
    case UT_OOM_ALWAYS:
      return NULL;
    case UT_OOM_NEVER:
      return malloc(size);
    case UT_OOM_NTH_FAIL:
      if (ut_alloc_counter - 1 == ut_oom_arg) {
        return NULL;
      } else {
        return malloc(size);
      }
    case UT_OOM_N_PASS:
      if (ut_alloc_counter - 1 <= ut_oom_arg) {
        return malloc(size);
      } else {
        return NULL;
      }
    default:
      assert(0 && "Bad ut_oom_policy_e value");
  }
  return malloc(size);
}

void ut_simulate_out_of_memory(struct dfk_t* dfk, ut_oom_policy_e policy, size_t arg)
{
  assert(dfk);
  dfk->malloc = ut_malloc;
  ut_oom_policy = policy;
  ut_oom_arg = arg;
  ut_alloc_counter = 0;
}

int ut_pyrun(const char* pyscript, int argc, char** argv)
{
  int retcode = 0;
  FILE* fp = fopen(pyscript, "r");
  if (!fp) {
    return -1;
  }
  wchar_t** wargv = malloc(argc * sizeof(wchar_t*));
  if (!wargv) {
    return -1;
  }
  for (int i = 0; i < argc; ++i) {
    wargv[i] = Py_DecodeLocale(argv[i], NULL);
  }
  PySys_SetArgv(argc, wargv);

  PyObject* m = PyImport_AddModule("__main__");
  if (m == NULL) {
    retcode = -1;
    goto ut_pyrun_cleanup;
  }
  PyObject* globals = PyModule_GetDict(m);

  PyObject* res = PyRun_FileExFlags(fp, pyscript,
      Py_file_input, globals, globals, 1, NULL);
  if (res == NULL) {
    PyErr_PrintEx(1);
    return -1;
  }

ut_pyrun_cleanup:
  if (wargv) {
    for (int i = 0; i < argc; ++i) {
      PyMem_RawFree(wargv[i]);
    }
    free(wargv);
  }
  return retcode;
}

typedef struct pyrun_async_task {
  pid_t pid;
} pyrun_async_task;

void* ut_pyrun_async(const char* pyscript, int argc, char** argv)
{
  pid_t pid = fork();
  if (pid == 0) {
    exit(ut_pyrun(pyscript, argc, argv));
  } else if (pid > 0) {
    pyrun_async_task* task = malloc(sizeof(pyrun_async_task));
    task->pid = pid;
    return task;
  } else {
    return NULL;
  }
}

int ut_pyrun_join(void* taskhandle)
{
  if (!taskhandle) {
    return -1;
  }
  pyrun_async_task* task = (pyrun_async_task*) taskhandle;

  int status;
  pid_t wres = waitpid(task->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
  if (wres < 0) {
    return -1;
  } else if (wres > 0) {
    /* Child exited */
    return (WIFEXITED(status) && !WEXITSTATUS(status)) ? 0 : -1;
  }
  /* Kill the child, hang until terminated */
  if (kill(task->pid, SIGTERM)) {
    return -1;
  }
  if (waitpid(task->pid, &status, WUNTRACED | WCONTINUED) < 0) {
    return -1;
  }
  return 0;
}

int ut_main(int argc, char** argv)
{
  Py_Initialize();
  ut_case_t* ut;
  const char* desired_group = NULL;
  const char* desired_name = NULL;
  if (argc > 1) {
    desired_group = argv[1];
  }
  if (argc > 2) {
    desired_name = argv[2];
  }
  ut = ut_all.head;
  while (ut) {
    printf("Running %s.%s\n", ut->group, ut->name);
    if ((desired_group && strcmp(desired_group, ut->group) != 0)
        || (desired_name && strcmp(desired_name, ut->name) != 0)) {
      ut_skip(ut->group, ut->name, "not this time");
      ut = ut->next;
      continue;
    }
    ut_running = ut;
    ut->func();
    printf("        %s.%s OK\n", ut->group, ut->name);
    ut = ut->next;
  }
  ut = ut_all.head;
  while (ut) {
    ut_case_t* next = ut->next;
    free(ut);
    ut = next;
  }
  Py_Finalize();
  return 0;
}

