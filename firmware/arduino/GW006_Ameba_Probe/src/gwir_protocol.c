#include "gwir_protocol.h"
#include <string.h>

uint16_t gwir_read_u16be(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }
uint32_t gwir_read_u32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
void gwir_write_u16be(uint8_t *p, uint16_t x) { p[0]=(uint8_t)(x>>8); p[1]=(uint8_t)x; }
void gwir_write_u32be(uint8_t *p, uint32_t x) {
    p[0]=(uint8_t)(x>>24); p[1]=(uint8_t)(x>>16); p[2]=(uint8_t)(x>>8); p[3]=(uint8_t)x;
}
uint32_t gwir_crc32_continue(uint32_t crc, const uint8_t *data, size_t size) {
    if (!data && size) return crc;
    for (size_t i=0; i<size; ++i) {
        crc ^= data[i];
        for (unsigned j=0; j<8; ++j) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc;
}
int gwir_encode(uint8_t *out, size_t cap, uint8_t cmd, uint32_t seq,
                const uint8_t *payload, uint32_t plen, size_t *out_size) {
    if (!out || !out_size || plen > GWIR_MAX_PAYLOAD || (plen && !payload)) return -1;
    const size_t needed = (size_t)GWIR_HEADER_SIZE + plen;
    if (cap < needed) return -1;
    memcpy(out, "GWIR", 4);
    out[4] = GWIR_VERSION; out[5] = cmd; out[6] = 0; out[7] = 0;
    gwir_write_u32be(out+8, seq);
    gwir_write_u32be(out+12, plen);
    gwir_write_u32be(out+16, 0);
    if (plen && payload != out+GWIR_HEADER_SIZE) memcpy(out+GWIR_HEADER_SIZE, payload, plen);
    uint32_t crc = gwir_crc32_continue(0xFFFFFFFFu, out, 16);
    crc = gwir_crc32_continue(crc, out+GWIR_HEADER_SIZE, plen) ^ 0xFFFFFFFFu;
    gwir_write_u32be(out+16, crc);
    *out_size = needed;
    return 0;
}
int gwir_decode(const uint8_t *in, size_t n, gwir_message *out) {
    if (!in || !out || n < GWIR_HEADER_SIZE) return -1;
    if (memcmp(in, "GWIR", 4) || in[4] != GWIR_VERSION || in[6] || in[7]) return -1;
    const uint32_t plen = gwir_read_u32be(in+12);
    if (plen > GWIR_MAX_PAYLOAD || (size_t)plen + GWIR_HEADER_SIZE != n) return -1;
    uint32_t crc = gwir_crc32_continue(0xFFFFFFFFu, in, 16);
    crc = gwir_crc32_continue(crc, in+GWIR_HEADER_SIZE, plen) ^ 0xFFFFFFFFu;
    if (crc != gwir_read_u32be(in+16)) return -1;
    out->command=in[5]; out->sequence=gwir_read_u32be(in+8);
    out->payload=in+GWIR_HEADER_SIZE; out->payload_len=plen;
    return 0;
}
