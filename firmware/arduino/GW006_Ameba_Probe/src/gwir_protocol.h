#ifndef GWIR_PROTOCOL_H
#define GWIR_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>

#define GWIR_VERSION 3u /* protocol upgrade: RX_READ now requires explicit RX_ACK */
#define GWIR_HEADER_SIZE 20u
#define GWIR_MAX_FRAMES_PER_BUTTON 5u
#define GWIR_MAX_PULSES_PER_FRAME 2048u
#define GWIR_FRAME_FIXED_BYTES 12u
#define GWIR_RX_FIXED_BYTES (4u + GWIR_FRAME_FIXED_BYTES) /* capture ID + TX-shaped data */
#define GWIR_MAX_PAYLOAD (GWIR_RX_FIXED_BYTES + GWIR_MAX_PULSES_PER_FRAME * 4u)
#define GWIR_MAX_MESSAGE (GWIR_HEADER_SIZE + GWIR_MAX_PAYLOAD)

/* The 96 button identities and their nonvolatile code storage belong to Android. */
enum gwir_command {
    GWIR_HELLO = 1,
    GWIR_RX_ARM = 2,
    GWIR_RX_DISARM = 3,
    GWIR_TX_FRAME = 4,
    GWIR_RX_READ = 5,
    GWIR_PING = 6,
    GWIR_RX_STATS = 7,
    GWIR_RX_ACK = 8, /* payload: uint32_t capture ID; only ACK removes frame */
    GWIR_HELLO_REPLY = 0x81,
    GWIR_RX_DATA = 0x85,
    GWIR_STATUS = 0x90,
    GWIR_PONG = 0x86,
    GWIR_RX_STATS_REPLY = 0x87
};
enum gwir_status {
    GWIR_OK = 0,
    GWIR_BAD_REQUEST = 1,
    GWIR_UNSUPPORTED = 2,
    GWIR_NO_CAPTURE = 3,
    GWIR_IR_FAILED = 4,
    GWIR_RANGE = 5,
    GWIR_BUSY = 6
};
typedef struct {
    uint8_t command;
    uint32_t sequence;
    const uint8_t *payload;  /* borrowed; lives as long as input buffer */
    uint32_t payload_len;
} gwir_message;

uint16_t gwir_read_u16be(const uint8_t *p);
uint32_t gwir_read_u32be(const uint8_t *p);
void gwir_write_u16be(uint8_t *p, uint16_t x);
void gwir_write_u32be(uint8_t *p, uint32_t x);
uint32_t gwir_crc32_continue(uint32_t crc, const uint8_t *data, size_t size);
int gwir_encode(uint8_t *out, size_t out_capacity, uint8_t command,
                uint32_t sequence, const uint8_t *payload, uint32_t payload_len,
                size_t *out_size);
int gwir_decode(const uint8_t *input, size_t input_size, gwir_message *out);
#endif
