/**
 * @file dfk/signal.h
 */

#pragma once
#include <dfk/context.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Suspend current fiber until signal arrives
 *
 * @todo Thread-safety
 * @todo Restore old signal handler
 */
int dfk_sigwait(dfk_t* dfk, int signum);

/**
 * Suspend current fiber until one of two signals arrives
 */
int dfk_sigwait2(dfk_t* dfk, int signum1, int signum2);

#ifdef __cplusplus
}
#endif

