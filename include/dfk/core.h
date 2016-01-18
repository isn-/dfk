#pragma once
#include <stddef.h>

typedef enum {
  dfk_err_ok = 0,
  dfk_err_out_of_memory,
  dfk_err_not_found,
  dfk_err_badarg,
  dfk_err_diff_context,
  dfk_err_sys,
  dfk_err_inprog
} dfk_error_e;

typedef enum {
  dfk_log_error = 0,
  dfk_log_info = 1,
  dfk_log_debug = 2
} dfk_log_e;


typedef struct {
  char* data;
  size_t size;
} dfk_iovec_t;

