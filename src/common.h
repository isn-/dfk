#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include <stddef.h>
#include <string.h>
#include <stdio.h>

typedef struct {
  char* data;
  size_t size;
} buf_t;

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DFK_MALLOC(ctx, nbytes) (ctx)->malloc((ctx)->userdata, nbytes)
#define DFK_FREE(ctx, p) (ctx)->free((ctx)->userdata, p)
#define DFK_REALLOC(ctx, p, nbytes) (ctx)->realloc((ctx)->userdata, p, nbytes)

#define DFK_LOG(ctx, channel, ...) \
if ((ctx)->log) {\
  char msg[512] = {0};\
  int printed;\
  printed = snprintf(msg, sizeof(msg), "%s (%s:%d) ", __FUNCTION__, __FILENAME__, __LINE__);\
  snprintf(msg + printed , sizeof(msg) - printed, __VA_ARGS__);\
  (ctx)->log((ctx)->userdata, channel, msg);\
}

#define DFK_ERROR(ctx, ...) DFK_LOG(ctx, 0, __VA_ARGS__)
#define DFK_INFO(ctx, ...) DFK_LOG(ctx, 1, __VA_ARGS__)

#ifdef DFK_ENABLE_DEBUG
#define DFK_DEBUG(ctx, ...) DFK_LOG(ctx, 2, __VA_ARGS__)
#else
#define DFK_DEBUG(...)
#endif

#pragma GCC diagnostic pop
