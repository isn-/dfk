/**
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 * Licensed under the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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


int ut_main(int argc, char** argv)
{
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
  return 0;
}

