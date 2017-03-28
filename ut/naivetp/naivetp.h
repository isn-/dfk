/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <stdint.h>
#include <dfk.h>

typedef struct _naivetp_server_t naivetp_server_t;

typedef enum naivetp_server_protocol_e {
  naivetp_server_echo = 0, /* send any received data back */
  naivetp_server_boor = 1 /* accept connection and close socket */
} naivetp_server_protocol_e;

/**
 * Naive transport protocol
 *
 * A text-based protocol designed for TCP socket testing.
 */
naivetp_server_t* naivetp_server_start(dfk_t* dfk, uint16_t port,
    naivetp_server_protocol_e protocol);
void naivetp_server_stop(naivetp_server_t*);

