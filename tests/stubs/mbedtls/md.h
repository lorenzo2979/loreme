#ifndef GWIR_TEST_ONLY_MBEDTLS_MD_H
#define GWIR_TEST_ONLY_MBEDTLS_MD_H
#include <stddef.h>
#include <stdint.h>
/* TEST ONLY. NEVER COPY THIS HEADER INTO REALTEK SDK. */
typedef struct { unsigned bytes; } mbedtls_md_info_t;
typedef enum { MBEDTLS_MD_SHA256 = 6 } mbedtls_md_type_t;
const mbedtls_md_info_t *mbedtls_md_info_from_type(mbedtls_md_type_t type);
unsigned char mbedtls_md_get_size(const mbedtls_md_info_t *md);
int mbedtls_md_hmac(const mbedtls_md_info_t *md, const unsigned char *key,
                    size_t keylen, const unsigned char *input,
                    size_t ilen, unsigned char *output);
#endif
