#include <stdlib.h>
#include <stdio.h>
#include <dfk/config.h>
#include <dfk/context.h>

static void log_message(int channel, const char* msg)
{
  printf("[%3d]: %.504s\n", channel, msg);
}

static dfk_context_t default_context = {
  malloc,
  free,
  realloc,
  log_message,
  DFK_PAGE_SIZE
};

dfk_context_t* dfk_default_context(void)
{
  return &default_context;
}

