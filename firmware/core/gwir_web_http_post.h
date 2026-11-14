#ifndef GWIR_WEB_HTTP_POST_H
#define GWIR_WEB_HTTP_POST_H
#include <stddef.h>
#include <stdint.h>
#include "gwir_auth.h"

/* Strict HTTP/1.1 POST /api/gwir header parser, suitable for a single
 * serialized Realtek STA listener. No TLS, nonce or key is generated here.
 * Require Host and Origin from the same configured local authority, reject
 * chunked/ambiguous lengths. Full body must be read separately and passed
 * unmodified to gwir_web_rpc_process which enforces signed GWAS framing.
 */
#define GWIR_WEB_HTTP_HEADER_MAX 2048u
int gwir_web_http_post_header(const uint8_t *headers, size_t header_len,
                              const char *expected_authority,
                              size_t *body_bytes);
#endif
