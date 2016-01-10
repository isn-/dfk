#pragma once
#include <stddef.h>

typedef enum {
  dfk_log_error = 0,
  dfk_log_info = 1,
  dfk_log_debug = 2
} dfk_log_e;


typedef struct {
  struct {
    struct dfk_coro_t* current_coro;
  } _ ;
  void* userdata;

  void* (*malloc) (void*, size_t);
  void (*free) (void*, void*);
  void* (*realloc)(void*, void*, size_t);
  void (*log)(void*, int, const char*);

  size_t default_coro_stack_size;

} dfk_context_t;


dfk_context_t* dfk_default_context(void);
