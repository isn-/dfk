#pragma once
#include <stddef.h>
#include <stdint.h>
#include <dfk/config.h>
#include <dfk/buf.h>
#include <dfk/context.h>

/**
 * @brief Extended buffer
 *
 * Contains additional metadata used by buffer manager.
 */
typedef struct {
  dfk_buf_t buf;
#ifdef DFK_ENABLE_MAGIC
  uint64_t magic;
#endif
  /**
   * @brief A moment in future when buffer should be released if not used
   *
   * A call to dfk_bufman_tick function will release expired buffers.
   */
  int64_t lifetime;
  /** @brief Null-terminated usage string */
  char usage[8];
  /** @brief 1 if is used somewhere outside buffer manager */
  unsigned int used : 1;
  /** @brief 1 if was released with do_cache == 0 */
  unsigned int released : 1;
} dfk_buf_ex_t;

int dfk_buf_ex_init(dfk_buf_ex_t* buf);
int dfk_buf_ex_free(dfk_buf_ex_t* buf);
int dfk_buf_ex_valid(dfk_buf_ex_t* buf);


/**
 * @brief Buffer manager housekeeping
 * @todo implement RB-tree lookup
 */
typedef struct dfk_bufman_hk_t {
  struct dfk_bufman_hk_t* next;
  struct dfk_bufman_hk_t* prev;
  /** @brief Buffer, that holds this dfk_bufman_hk_t object */
  dfk_buf_ex_t self;
  /** @brief Number of buffers in @ref buffers array */
  size_t nbuffers;
  dfk_buf_ex_t buffers[1];
} dfk_bufman_hk_t;

int dfk_bufman_hk_init(dfk_bufman_hk_t* hk, dfk_buf_t* buf);
int dfk_bufman_hk_empty(dfk_bufman_hk_t* hk);

