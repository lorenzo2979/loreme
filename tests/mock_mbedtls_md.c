/* HOST TEST ONLY: emulate Mbed TLS calls using OpenSSL to test glue interface. */
#include "mbedtls/md.h"
#include <openssl/hmac.h>
static mbedtls_md_info_t sha256 = {32u};
int gwir_mock_md_fail = 0;
const mbedtls_md_info_t *mbedtls_md_info_from_type(mbedtls_md_type_t type) {
    return (type == MBEDTLS_MD_SHA256 && !gwir_mock_md_fail) ? &sha256 : NULL;
}
unsigned char mbedtls_md_get_size(const mbedtls_md_info_t *md) {
    return md ? (unsigned char)md->bytes : 0u;
}
int mbedtls_md_hmac(const mbedtls_md_info_t *md, const unsigned char *key,
                    size_t keylen, const unsigned char *input,
                    size_t ilen, unsigned char *output) {
    unsigned len = 0u;
    if (!md || gwir_mock_md_fail || keylen > 64u ||
        !HMAC(EVP_sha256(), key, (int)keylen, input, ilen, output, &len) ||
        len != 32u) return -1;
    return 0;
}
