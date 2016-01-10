#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dfk/core.h>
#include <dfk/config.h>
#include <dfk/context.h>

static void default_log(void* ud, int channel, const char* msg)
{
  char strchannel[5] = {0};
  (void) ud;
  switch(channel) {
    case dfk_log_error: memcpy(strchannel, "error", 5); break;
    case dfk_log_info: memcpy(strchannel, " info", 5); break;
    case dfk_log_debug: memcpy(strchannel, "debug", 5); break;
    default: snprintf(strchannel, sizeof(strchannel), "%5d", channel);
  }
  printf("[%.5s] %.503s\n", strchannel, msg);
}

static void* default_malloc(void* ud, size_t size)
{
  (void) ud;
  return malloc(size);
}

static void default_free(void* ud, void* p)
{
  (void) ud;
  free(p);
}

static void* default_realloc(void* ud, void* p, size_t size)
{
  (void) ud;
  return realloc(p, size);
}

static dfk_context_t default_context = {
  {NULL},
  NULL,
  default_malloc,
  default_free,
  default_realloc,
  default_log,
  DFK_DEFAULT_STACK_SIZE
};

dfk_context_t* dfk_default_context(void)
{
  return &default_context;
}

