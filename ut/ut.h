#pragma once
#include <string.h>
#include "ut_init.h"

void ut_register_all_test_cases(void);
void ut_register_test_case(const char* group, const char* name, void (*func)(void));
void ut_error(const char* file, const int line, const char* message);

#define TEST(group, name) void ut_##group##_##name(void)
#define TEST_F(group, name) \
void _ut_##group##_##name(fixture*); \
void ut_##group##_##name(void) \
{ \
  fixture f; \
  memset(&f, 0, sizeof(f)); \
  fixture_setup(&f); \
  _ut_##group##_##name(&f); \
  fixture_teardown(&f); \
} \
\
void _ut_##group##_##name(fixture* fixture)

#define EXPECT(expr) \
if (!(expr)) { \
  ut_error(__FILE__, __LINE__, #expr); \
}

#define ASSERT(expr) \
if (!(expr)) { \
  ut_error(__FILE__, __LINE__, #expr); \
  return; \
}

