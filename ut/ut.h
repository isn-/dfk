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

#pragma once
#include <string.h>
#include "ut_init.h"

void ut_register_all_test_cases(void);
void ut_register_test_case(const char* group, const char* name, void (*func)(void));
void ut_error(const char* file, const int line, const char* message);
void ut_disable(const char* group, const char* name);

#define TEST(group, name) void ut_##group##_##name(void)
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

#define DISABLED_TEST(group, name) \
void _ut_##group##_##name(void); \
void ut_##group##_##name(void) \
{ \
  (void) _ut_##group##_##name; \
  ut_disable(DFK_STRINGIFY(group), DFK_STRINGIFY(name)); \
} \
\
void _ut_##group##_##name(void)

#define DISABLED_TEST_F(fixture_name, group, name) \
void _ut_##group##_##name(fixture_name##_t*); \
void ut_##group##_##name(void) \
{ \
  (void) fixture_name##_setup; \
  (void) fixture_name##_teardown; \
  (void) _ut_##group##_##name; \
  ut_disable(DFK_STRINGIFY(group), DFK_STRINGIFY(name)); \
} \
\
void _ut_##group##_##name(fixture_name##_t* fixture)

#define EXPECT(expr) \
if (!(expr)) { \
  ut_error(__FILE__, __LINE__, #expr); \
}

#define ASSERT_RET(expr, ret) \
if (!(expr)) { \
  ut_error(__FILE__, __LINE__, #expr); \
  return (ret); \
}

#define ASSERT(expr) \
if (!(expr)) { \
  ut_error(__FILE__, __LINE__, #expr); \
  return; \
}

#define EXPECT_OK(expr) EXPECT((expr) == dfk_err_ok)
#define ASSERT_OK(expr) ASSERT((expr) == dfk_err_ok)
