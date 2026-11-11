#ifndef GWIR_SDK_HMAC_H
#define GWIR_SDK_HMAC_H
#include <stddef.h>
#include <stdint.h>
#include "../core/gwir_auth.h"

/* Realtek SDK Mbed TLS binding for GWAS v1; no key provisioning or Wi-Fi. */
#define GWIR_SDK_HMAC_REJECT (-1)
#define GWIR_SDK_HMAC_CRYPTO_ERROR (-2)
int gwir_sdk_mbedtls_hmac(void *ctx, const uint8_t *key, size_t key_len,
                          const uint8_t *data, size_t data_len,
                          uint8_t out[GWIR_AUTH_TAG_BYTES]);
#endif
