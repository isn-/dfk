/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/string_map.hpp>

namespace dfk {

StringMap::StringMap(dfk_strmap_t* strmap)
  : Wrapper(strmap)
{
}

StringMap::StringMap(const dfk_strmap_t* strmap)
  : Wrapper(const_cast<dfk_strmap_t*>(strmap))
{
}

} // namespace dfk
