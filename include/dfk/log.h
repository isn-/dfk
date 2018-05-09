/**
 * @file dfk/log.h
 * Logging facilities
 *
 * @copyright
 * Copyright (c) 2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Logging stream
 *
 * Also could be considered as logging level.
 */
typedef enum dfk_log_e {
  /**
   * Error messages
   *
   * An inevitable error has occured. Manual intervention is needed
   * to recover.
   */
  dfk_log_error = 0,

  /**
   * Warning message
   *
   * Normal execution path has been violated, although library knows
   * how to recover from it. E.g. TCP connection was unexpectedly
   * closed by remote peer.
   *
   * Presence of warning messages in the log may indicate that some
   * extra configuration of external systems, or inside dfk library
   * itself is required.
   */
  dfk_log_warning = 1,

  /**
   * Informational message
   *
   * An event that might be useful for system monitoring has been occured.
   */

  dfk_log_info = 2,
  /**
   * Debug messages
   *
   * A very verbose logging level. Disabled by default, can be enabled
   * by setting DFK_DEBUG compile-time flag.
   */
  dfk_log_debug = 3
} dfk_log_stream;

#ifdef __cplusplus
}
#endif

