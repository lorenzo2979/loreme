#ifndef GWIR_ENGINE_H
#define GWIR_ENGINE_H
#include "gwir_protocol.h"

typedef struct {
    /* Return 0 on success; nonzero means hardware failure.
       Durations alternate MARK and SPACE, starting with MARK.
       gap_after_us is handled AFTER emitting this one IR frame. */
    int (*tx)(void *ctx, uint32_t carrier_hz, uint16_t duty_permille,
              const uint32_t *durations_us, uint16_t count, uint32_t gap_after_us);
    /* Must arm/disarm receive hardware without overwriting pending captures. */
    int (*rx_arm)(void *ctx);
    int (*rx_disarm)(void *ctx);
    void *ctx;
} gwir_ir_hal;

typedef struct {
    uint32_t capture_id; /* unique within the current device uptime, nonzero */
    uint32_t carrier_hz; /* configured estimate, NOT measured by a demodulator */
    uint16_t duty_permille;
    uint16_t count;
    uint32_t gap_after_us;
    uint32_t durations_us[GWIR_MAX_PULSES_PER_FRAME];
} gwir_rx_frame;

typedef struct {
    gwir_ir_hal hal;
    uint8_t rx_armed;
    uint8_t rx_head;
    uint8_t rx_count;
    uint16_t rx_dropped; /* saturates at UINT16_MAX */
    uint32_t next_capture_id; /* increments across RX_ARM sessions */
    uint32_t last_ack_id; /* allows harmless duplicate ACK */
    gwir_rx_frame rx_queue[GWIR_MAX_FRAMES_PER_BUTTON];
    uint32_t tx_durations[GWIR_MAX_PULSES_PER_FRAME];
} gwir_engine;

/* Large (~50KB) object: allocate statically in target firmware, NOT on a small FreeRTOS task stack. */
void gwir_engine_init(gwir_engine *e, gwir_ir_hal hal);
/* Must run in same serialized task as process(), or with appropriate locking.
   Never call directly from a hardware interrupt; enqueue timings to a worker task. */
int gwir_engine_capture(gwir_engine *e, uint32_t carrier_hz, uint16_t duty_permille,
                        const uint32_t *durations_us, uint16_t count, uint32_t gap_after_us);
/* Output is one complete GWIR response of at most GWIR_MAX_MESSAGE bytes. */
int gwir_engine_process(gwir_engine *e, const uint8_t *request, size_t request_size,
                        uint8_t *response, size_t response_cap, size_t *response_size);
#endif
