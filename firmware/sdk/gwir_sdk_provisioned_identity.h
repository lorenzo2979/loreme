#ifndef GWIR_SDK_PROVISIONED_IDENTITY_H
#define GWIR_SDK_PROVISIONED_IDENTITY_H
#include <stdint.h>
#include "gwir_sdk_secure_start.h"
/* Board-backed device identity: no default key, RNG or flash slot.
 * reserve_epoch MUST persist a strictly increasing nonzero value before
 * returning; random8 MUST use audited TRNG/CSPRNG; deliver_secure MUST be
 * out-of-band mutually authenticated (NEVER open HTTP or broadcast).
 */
typedef struct {
    void *ctx;
    int (*approved)(void *ctx);
    int (*read_device_key)(void *ctx, uint8_t key[GWIR_AUTH_KEY_BYTES]);
    int (*reserve_epoch)(void *ctx, uint64_t *persisted_epoch);
    int (*random8)(void *ctx, uint8_t out[8]);
    int (*deliver_secure)(void *ctx, const uint8_t nonce[GWIR_AUTH_NONCE_BYTES]);
} gwir_sdk_identity_io;
typedef struct {
    gwir_sdk_identity_io io;
    uint8_t expected_nonce[GWIR_AUTH_NONCE_BYTES];
    uint8_t stage;
} gwir_sdk_identity_state;
int gwir_sdk_identity_init(gwir_sdk_identity_state *state,
                           const gwir_sdk_identity_io *io,
                           gwir_sdk_secure_provider *provider);
void gwir_sdk_identity_wipe(gwir_sdk_identity_state *state);
#endif
