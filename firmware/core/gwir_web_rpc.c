#include "gwir_web_rpc.h"
#include <string.h>

typedef struct {
    uint8_t *bytes;
    size_t capacity;
    size_t used;
    unsigned replies;
} gwir_web_rpc_sink;

/* Existing owner serializes its authenticated stream and sends only signed
 * GWAS records. Keep the bridge unable to emit an unsigned success. */
static int gather_signed(void *context, const uint8_t *data, size_t len)
{
    gwir_web_rpc_sink *sink = (gwir_web_rpc_sink *)context;
    if (!sink || !sink->bytes || !data ||
        sink->replies != 0u || len > sink->capacity ||
        len < GWIR_AUTH_HEADER_BYTES + GWIR_HEADER_SIZE + GWIR_AUTH_TAG_BYTES ||
        len > GWIR_AUTH_MAX_PACKET ||
        memcmp(data, "GWAS", 4u) != 0 ||
        data[4] != GWIR_AUTH_VERSION ||
        data[5] != GWIR_AUTH_FROM_DEVICE ||
        data[6] != 0u || data[7] != 0u) return -1;
    uint32_t inner = gwir_read_u32be(data + 32u);
    if (inner < GWIR_HEADER_SIZE || inner > GWIR_MAX_MESSAGE ||
        len != GWIR_AUTH_HEADER_BYTES + (size_t)inner + GWIR_AUTH_TAG_BYTES)
        return -1;
    /* The signature is verified by the browser, not by trusting HTTP 200. */
    memmove(sink->bytes, data, len);
    sink->used = len;
    sink->replies = 1u;
    return 0;
}

int gwir_web_rpc_process(gwir_runtime_owner *owner,
                         const uint8_t *request, size_t request_len,
                         uint8_t *response, size_t response_cap,
                         size_t *response_len)
{
    gwir_web_rpc_sink sink;
    int completed;
    uint32_t inner_length;
    if (response_len) *response_len = 0u;
    if (!owner || !owner->initialized || !owner->rx.active ||
        !owner->tx.active || owner->connected || !owner->engine ||
        !owner->scratch || !owner->signed_response || !request ||
        !response || !response_len || response_cap < GWIR_AUTH_MAX_PACKET ||
        response == owner->signed_response ||
        request_len < GWIR_AUTH_HEADER_BYTES + GWIR_HEADER_SIZE + GWIR_AUTH_TAG_BYTES ||
        request_len > GWIR_AUTH_MAX_PACKET) return -1;

    /* Strictly one GWAS envelope, never pipelined requests or a truncated
     * frame. Do this BEFORE any authenticated command can have side effects.
     * The HMAC, direction, nonce, counter and inner CRC are then verified
     * by the existing owner, not by this size-only envelope preflight. */
    if (memcmp(request, "GWAS", 4u) != 0 ||
        request[4] != GWIR_AUTH_VERSION ||
        request[5] != GWIR_AUTH_TO_DEVICE ||
        request[6] != 0u || request[7] != 0u) return -2;
    inner_length = gwir_read_u32be(request + 32u);
    if (inner_length < GWIR_HEADER_SIZE || inner_length > GWIR_MAX_MESSAGE ||
        request_len != GWIR_AUTH_HEADER_BYTES + (size_t)inner_length +
                       GWIR_AUTH_TAG_BYTES) return -2;

    if (gwir_runtime_owner_connect(owner) != 0) return -3;
    sink.bytes = response;
    sink.capacity = response_cap;
    sink.used = 0u;
    sink.replies = 0u;
    completed = gwir_runtime_owner_feed(owner, request, request_len,
                                        gather_signed, &sink);
    gwir_runtime_owner_disconnect(owner);
    if (completed != 1 || sink.replies != 1u ||
        sink.used < GWIR_AUTH_HEADER_BYTES + GWIR_HEADER_SIZE +
                    GWIR_AUTH_TAG_BYTES) {
        /* Never return a partial signed frame as an HTTP success. */
        if (sink.used) memset(response, 0, sink.used);
        return -4;
    }
    *response_len = sink.used;
    return 0;
}
