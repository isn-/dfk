/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <assert.h>
#include <sys/uio.h>
#include <dfk/misc.h>

size_t dfk_buf_sizeof(void)
{
  return sizeof(dfk_buf_t);
}

size_t dfk_iovec_sizeof(void)
{
  return sizeof(dfk_iovec_t);
}

void dfk_buf_append(dfk_buf_t* to, const char* data, size_t size)
{
  assert(to);
  if (to->data) {
    assert(to->data + to->size == data);
    to->size += size;
  } else {
    to->data = (char*) data;
    to->size = size;
  }
}

