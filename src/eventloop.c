/**
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk/eventloop.h>
#include <dfk/internal.h>

size_t dfk__io_events_to_str(int events, char* buf, size_t buflen)
{
  struct {
    int event;
    int len;
    const char* str;
  } all_events[] = {
    {DFK_IO_IN, 2, "IN"},
    {DFK_IO_OUT, 3, "OUT"},
    {DFK_IO_ERR, 3, "ERR"},
  };
  char* end = buf + buflen;
  char* orig_buf = buf;
  int first = 1;
  for (size_t i = 0; i < DFK_SIZE(all_events); ++i) {
    if (events & all_events[i].event) {
      /* Not enough space in destination buffer */
      if (end - buf < all_events[i].len + !first) {
        break;
      }
      if (!first) {
        *buf++ = '|';
      }
      memcpy(buf, all_events[i].str, all_events[i].len);
      buf += all_events[i].len;
      first = 0;
    }
  }
  return buf - orig_buf;
}
