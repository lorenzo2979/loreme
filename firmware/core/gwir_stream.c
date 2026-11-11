#include "gwir_stream.h"
#include <limits.h>
#include <string.h>

void gwir_stream_init(gwir_stream *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
}

static int reject(gwir_stream *s) {
    if (s->messages_invalid != UINT32_MAX) ++s->messages_invalid;
    s->failed = 1;
    return -1;
}

int gwir_stream_feed(gwir_stream *s, const uint8_t *input, size_t length,
                     gwir_stream_handler handler, void *context) {
    if (!s || !handler || (length && !input) || s->failed) return -1;
    int completed = 0;
    size_t offset = 0;
    while (offset < length) {
        /* Only read an untrusted payload length once all 20 header bytes
           have arrived, after checking magic/version/reserved bytes. */
        size_t target = s->expected ? s->expected : GWIR_HEADER_SIZE;
        if (s->filled > target) return reject(s);
        size_t space = target - s->filled;
        size_t available = length - offset;
        size_t take = space < available ? space : available;
        if (take) {
            memcpy(s->buffer + s->filled, input + offset, take);
            s->filled += take;
            offset += take;
        }
        if (s->filled < target) break;
        if (!s->expected) {
            const uint8_t *b = s->buffer;
            if (memcmp(b, "GWIR", 4) || b[4] != GWIR_VERSION || b[6] || b[7])
                return reject(s);
            uint32_t plen = gwir_read_u32be(b+12);
            if (plen > GWIR_MAX_PAYLOAD) return reject(s);
            s->expected = GWIR_HEADER_SIZE + (size_t)plen;
            if (s->expected != s->filled) continue;
        }
        if (s->filled == s->expected) {
            gwir_message msg;
            if (gwir_decode(s->buffer, s->filled, &msg)) return reject(s);
            if (handler(context, s->buffer, s->filled)) return reject(s);
            if (s->messages_ok != UINT32_MAX) ++s->messages_ok;
            ++completed;
            s->filled = 0;
            s->expected = 0;
        }
    }
    return completed;
}
