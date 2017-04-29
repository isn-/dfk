/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/context.h>
#include <ut.h>

TEST(context, sizeof)
{
  EXPECT(dfk_sizeof() == sizeof(dfk_t));
}
