#ifndef GW006_AMEBA_HMAC_H
#define GW006_AMEBA_HMAC_H
#include "gwir_auth.h"
#ifdef __cplusplus
extern "C" {
#endif
int gw006_ameba_hmac_sha256(void *ctx, const uint8_t *key, size_t key_len,
                            const uint8_t *data, size_t data_len,
                            uint8_t output[GWIR_AUTH_TAG_BYTES]);
#ifdef __cplusplus
}
#endif
#endif
