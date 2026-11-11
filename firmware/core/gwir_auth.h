#ifndef GWIR_AUTH_H
#define GWIR_AUTH_H
#include <stddef.h>
#include <stdint.h>
#include "gwir_protocol.h"

/* GWAS v1: authenticated envelope *around* a complete GWIR v3 message.
 * No public Wi-Fi listener is provided. Keys/nonce must be provisioned at
 * runtime by a secure board/app-specific mechanism; NEVER compile secrets in.
 * A fresh unpredictable boot nonce is REQUIRED on every device reboot.
 */
#define GWIR_AUTH_VERSION 1u
#define GWIR_AUTH_KEY_BYTES 32u
#define GWIR_AUTH_NONCE_BYTES 16u
#define GWIR_AUTH_TAG_BYTES 32u
#define GWIR_AUTH_HEADER_BYTES 36u
#define GWIR_AUTH_MAX_PACKET (GWIR_AUTH_HEADER_BYTES + GWIR_MAX_MESSAGE + GWIR_AUTH_TAG_BYTES)
#define GWIR_AUTH_MAX_COUNTER UINT64_C(0x7fffffffffffffff)
#define GWIR_AUTH_TO_DEVICE 1u
#define GWIR_AUTH_FROM_DEVICE 2u

/* Callback must compute HMAC-SHA256 using a vetted cryptographic library.
 * 0 on success; nonzero if unavailable. No fallback to CRC32 permitted. */
typedef int (*gwir_auth_hmac_fn)(void *ctx, const uint8_t *key,
                                 size_t key_len, const uint8_t *data,
                                 size_t data_len, uint8_t out[GWIR_AUTH_TAG_BYTES]);
typedef struct {
    uint8_t key[GWIR_AUTH_KEY_BYTES];
    uint8_t boot_nonce[GWIR_AUTH_NONCE_BYTES];
    uint64_t last_rx_counter;
    uint64_t tx_counter;
    gwir_auth_hmac_fn hmac;
    void *crypto_ctx;
    uint8_t active;
} gwir_auth_session;

enum gwir_auth_result {
    GWIR_AUTH_OK = 0,
    GWIR_AUTH_REJECT = -1,
    GWIR_AUTH_CRYPTO_ERROR = -2,
    GWIR_AUTH_REPLAY = -3
};

/* Input key/nonce are copied. Reject all-zero key/nonce and absent HMAC.
 * Caller MUST use strong RNG for 16-byte boot nonce; this does not make RNG. */
int gwir_auth_init(gwir_auth_session *s, const uint8_t key[GWIR_AUTH_KEY_BYTES],
                   const uint8_t boot_nonce[GWIR_AUTH_NONCE_BYTES],
                   gwir_auth_hmac_fn hmac, void *crypto_ctx);
/* Entire encoded record is authenticated, including direction, nonce,
 * counter, length, GWIR header/CRC and payload. Output buffers cannot alias. */
int gwir_auth_wrap(gwir_auth_session *s, uint8_t direction,
                   const uint8_t *inner, size_t inner_len,
                   uint8_t *out, size_t cap, size_t *out_len);
/* On success, inner points into the input and MUST NOT outlive it.
 * Each direction must have a separate RX session to enforce its counter.
 * Replay counter advances only after HMAC and inner GWIR CRC both verify. */
int gwir_auth_unwrap(gwir_auth_session *s, uint8_t expected_direction,
                     const uint8_t *record, size_t size,
                     const uint8_t **inner, size_t *inner_len);
void gwir_auth_clear(gwir_auth_session *s);
#endif
