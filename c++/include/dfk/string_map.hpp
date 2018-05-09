/**
 * @file dfk/string_map.hpp
 *
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once
#include <dfk/strmap.h>
#include <dfk/wrapper.hpp>

namespace dfk {

class StringMap : public Wrapper<dfk_strmap_t, dfk_strmap_sizeof>
{
public:
  explicit StringMap(dfk_strmap_t* strmap);
  explicit StringMap(const dfk_strmap_t* strmap);
};

} // namespace dfk
