#ifndef GWIR_AUTH_DISPATCH_H
#define GWIR_AUTH_DISPATCH_H
#include "gwir_auth.h"
#include "gwir_engine.h"
/* All network entry points MUST pass through an authenticated dispatcher.
 * A future TCP service must NOT expose gwir_engine_process directly.
 * Caller owns two persistent per-boot session contexts (rx and tx), and two
 * separate static response buffers (>= GWIR_MAX_MESSAGE and
 * GWIR_AUTH_MAX_PACKET respectively). Never put these on a FreeRTOS stack.
 * No Wi-Fi listener is created by this module. */
int gwir_auth_dispatch(gwir_auth_session *rx, gwir_auth_session *tx,
                       gwir_engine *engine, const uint8_t *request, size_t req_size,
                       uint8_t *scratch, size_t scratch_cap,
                       uint8_t *response, size_t response_cap,
                       size_t *response_len);
#endif
