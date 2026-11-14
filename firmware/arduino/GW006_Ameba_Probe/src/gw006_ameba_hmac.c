#include "gw006_ameba_hmac.h"
#include <mbedtls/md.h>
#include <string.h>

/* Use the official Realtek mbedTLS SHA-256 HMAC backend. There is NO test
 * key provisioned into a GWIR owner and no CRC fallback for authentication. */
int gw006_ameba_hmac_sha256(void *ctx, const uint8_t *key, size_t key_len,
                            const uint8_t *data, size_t data_len,
                            uint8_t output[GWIR_AUTH_TAG_BYTES]) {
    (void)ctx;
    if (!key || !data || !output || key_len != GWIR_AUTH_KEY_BYTES ||
        data_len == 0u || data_len > GWIR_AUTH_MAX_PACKET) return -1;
    const mbedtls_md_info_t *md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md || mbedtls_md_get_size(md) != GWIR_AUTH_TAG_BYTES) {
        memset(output, 0, GWIR_AUTH_TAG_BYTES);
        return -2;
    }
    if (mbedtls_md_hmac(md, key, key_len, data, data_len, output) != 0) {
        memset(output, 0, GWIR_AUTH_TAG_BYTES);
        return -2;
    }
    return 0;
}
