#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dfk.h>

#include "ut.h"

typedef struct ut_case_t {
  const char* group;
  const char* name;
  void (*func)(void);
  struct ut_case_t* next;
} ut_case_t;

typedef struct {
  ut_case_t* head;
} ut_case_list_t;

typedef struct {
  int failed;
  const char* file;
  int line;
  const char* expr;
} ut_last_error_t;

static void ut_last_error_reset(ut_last_error_t* err)
{
  err->failed = 0;
  err->file = NULL;
  err->line = 0;
  err->expr = NULL;
}

ut_case_list_t ut_all;
static ut_last_error_t last_error;

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
  last_error.failed = 1;
  last_error.file = file;
  last_error.line = line;
  last_error.expr = message;
}


int main(int argc, char** argv)
{
  ut_case_t* ut;
  int ret = 0;
  const char* desired_group = NULL;
  const char* desired_name = NULL;
  (void) argc;
  (void) argv;
  ut_register_all_test_cases();
  if (argc > 1) {
    desired_group = argv[1];
  }
  if (argc > 2) {
    desired_name = argv[2];
  }
  ut = ut_all.head;
  while (ut) {
    if ((desired_group && strcmp(desired_group, ut->group) != 0)
        || (desired_name && strcmp(desired_name, ut->name) != 0)) {
      printf("Skipping %s.%s ...\n", ut->group, ut->name);
      ut = ut->next;
      continue;
    }
    printf("Running %s.%s\n", ut->group, ut->name);
    ut_last_error_reset(&last_error);
    ut->func();
    if (last_error.failed) {
      printf("        %s.%s FAIL (%s:%d): assertion failed: \"%s\"\n",
          ut->group, ut->name,
          last_error.file, last_error.line, last_error.expr);
      ret = 1;
    } else {
      printf("        %s.%s OK\n", ut->group, ut->name);
    }
    ut = ut->next;
  }
  return ret;
}

