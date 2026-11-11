#include "gwir_sdk_hmac.h"
#include <mbedtls/md.h>

/* No secret or random value is embedded in firmware. This is only an HMAC
 * provider for the caller-provisioned, in-memory GWAS session. Provisioning,
 * nonce generation and persistent anti-replay policy are separate work.
 * No fallback to CRC or unsecured network operation is provided.
 */
static void wipe(uint8_t *p, size_t n) {
    volatile uint8_t *v = (volatile uint8_t *)p;
    while (n--) *v++ = 0u;
}

int gwir_sdk_mbedtls_hmac(void *ctx, const uint8_t *key, size_t key_len,
                          const uint8_t *data, size_t data_len,
                          uint8_t out[GWIR_AUTH_TAG_BYTES]) {
    (void)ctx;
    if (!key || !data || !out ||
        key_len != GWIR_AUTH_KEY_BYTES ||
        data_len == 0u || data_len > GWIR_AUTH_MAX_PACKET)
        return GWIR_SDK_HMAC_REJECT;

    /* Use the SDK's configured digest implementation, rather than a bundled
     * cryptographic implementation or a software fallback. */
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md || mbedtls_md_get_size(md) != GWIR_AUTH_TAG_BYTES) {
        wipe(out, GWIR_AUTH_TAG_BYTES);
        return GWIR_SDK_HMAC_CRYPTO_ERROR;
    }
    if (mbedtls_md_hmac(md, key, key_len, data, data_len, out) != 0) {
        wipe(out, GWIR_AUTH_TAG_BYTES);
        return GWIR_SDK_HMAC_CRYPTO_ERROR;
    }
    return 0;
}
