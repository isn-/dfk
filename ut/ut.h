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

#pragma once
#include <string.h>
#include <dfk.h>
#include <ut_init.h>

void ut_register_test_case(const char* group, const char* name,
                           void (*func)(void));
void ut_error(const char* file, const int line, const char* message);
void ut_skip(const char* group, const char* name, const char* reason);
int ut_main(int argc, char** argv);

typedef enum ut_oom_policy_e {
  UT_OOM_ALWAYS = 0,
  UT_OOM_NEVER = 1,
  UT_OOM_NTH_FAIL = 2,
  UT_OOM_N_PASS = 3,
} ut_oom_policy_e;

void ut_simulate_out_of_memory(struct dfk_t* dfk, ut_oom_policy_e policy,
                               size_t arg);


/**
 * Unit test declaration macros
 */

/**
 * Declares a simple test
 */
#define TEST(group, name) void ut_##group##_##name(void)

/**
 * Declares a test with test fixture
 */
#define TEST_F(fixture_name, group, name) \
void _ut_##group##_##name(fixture_name##_t*); \
void ut_##group##_##name(void) \
{ \
  fixture_name##_t f; \
  memset(&f, 0, sizeof(f)); \
  fixture_name##_setup(&f); \
  _ut_##group##_##name(&f); \
  fixture_name##_teardown(&f); \
} \
\
void _ut_##group##_##name(fixture_name##_t* fixture)

/**
 * Declares a simple test, which is temporary excluded from the test suite
 */
#define DISABLED_TEST(group, name) \
void _ut_##group##_##name(void); \
void ut_##group##_##name(void) \
{ \
  (void) _ut_##group##_##name; \
  ut_skip(DFK_STRINGIFY(group), DFK_STRINGIFY(name), "test is disabled"); \
} \
\
void _ut_##group##_##name(void)


/**
 * Declares a test with test fixture, which is temporary excluded from the test
 * suite
 */
#define DISABLED_TEST_F(fixture_name, group, name) \
void _ut_##group##_##name(fixture_name##_t*); \
void ut_##group##_##name(void) \
{ \
  (void) fixture_name##_setup; \
  (void) fixture_name##_teardown; \
  (void) _ut_##group##_##name; \
  ut_skip(DFK_STRINGIFY(group), DFK_STRINGIFY(name), "test is disabled"); \
} \
\
void _ut_##group##_##name(fixture_name##_t* fixture)


/**
 * Unit test assertion macros
 */


/**
 * Expect expr to be evaluated to non-zero
 */
#define EXPECT(expr) \
if (!(expr)) { \
  ut_error(__FILE__, __LINE__, #expr); \
}


/**
 * Expect exptr to be evaluated to dfk_err_ok
 */
#define EXPECT_OK(expr) EXPECT((expr) == dfk_err_ok)


/**
 * Expect dfk_buf_t-compatible object buf to be equal to NULL-terminated str
 */
#define EXPECT_BUFSTREQ(buf, str) \
{ \
  EXPECT((buf).size == strlen((str))); \
  EXPECT(!strncmp((buf).data, (str), strlen((str)))); \
}
