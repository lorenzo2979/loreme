#ifndef GWIR_WEB_RPC_H
#define GWIR_WEB_RPC_H
#include <stddef.h>
#include <stdint.h>
#include "gwir_runtime_owner.h"

/* One browser HTTP POST body = exactly ONE signed GWAS v1 envelope.
 * This is a transport adapter, not a public unauthenticated IR endpoint.
 * The caller MUST enforce a bounded HTTP request, same-origin policy,
 * private STA-only binding and a single serialized owner.
 *
 * The owner is created by the audited per-device key/nonce provider.
 * No key, nonce or counter is reset on HTTP connection boundaries.
 * Response is a signed GWAS envelope for the browser's WebCrypto client.
 * Caller provides distinct static buffers, not large task-stack buffers.
 *
 * 0 means a complete signed answer; negative means reject and send NO
 * unsigned IR details or untrusted success result. No flash/pin access here.
 */
int gwir_web_rpc_process(gwir_runtime_owner *owner,
                         const uint8_t *request, size_t request_len,
                         uint8_t *response, size_t response_cap,
                         size_t *response_len);
#endif
