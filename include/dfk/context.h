#pragma once
#include <stddef.h>

typedef struct {
  void* (*malloc) (size_t);
  void (*free) (void*);
  void* (*realloc)(void*, size_t);
  void (*logger)(int, const char*);

  size_t bufman_housekeeping_buffer_size;
} dfk_context_t;


dfk_context_t* dfk_default_context(void);

