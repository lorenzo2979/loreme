#ifndef GWIR_AMEBAD_RX_FIFO_H
#define GWIR_AMEBAD_RX_FIFO_H
#include <stdint.h>
#include "../core/gwir_capture_builder.h"

/* The peripheral has a 32-word HW FIFO. This larger software queue is filled
 * by its IRQ handler and drained by ONE FreeRTOS receiver worker. The worker
 * must mask the **IR IRQ** before reading the queue and unmask afterwards.
 * No GWIR engine operations or heap allocation are permitted in an ISR.
 * The caller must confirm the UART/IR pinmux and use a supported RX pin. */
#define GWIR_AMEBAD_RX_SW_DEPTH 512u
#define GWIR_AMEBAD_RX_COUNTER_MASK 0x7fffffffu
#define GWIR_AMEBAD_RX_LEVEL_HIGH 0x80000000u

typedef enum {
    GWIR_RX_WORD=1,
    GWIR_RX_IDLE=2,
    GWIR_RX_ERROR=3,
    GWIR_RX_RESYNC=4
} gwir_amebad_rx_event_type;
typedef struct {
    uint32_t value;
    uint8_t type;
} gwir_amebad_rx_event;
typedef struct {
    gwir_amebad_rx_event events[GWIR_AMEBAD_RX_SW_DEPTH];
    uint16_t rd;
    uint16_t wr;
    uint16_t used;
    uint8_t dropping; /* Producer drops until its next RX idle interrupt. */
    uint32_t overflows;
} gwir_amebad_rx_queue;
typedef struct {
    /* Called from task context; must mask / unmask exactly the IR IRQ. */
    void (*lock_ir_irq)(void *ctx);
    void (*unlock_ir_irq)(void *ctx);
    void *ctx;
} gwir_amebad_rx_lock;
typedef struct {
    gwir_amebad_rx_queue *queue;
    gwir_capture_builder builder;
    gwir_amebad_rx_lock lock;
    uint32_t sample_hz;
    uint32_t last_overflows;
    uint32_t invalid_events;
    uint32_t decoded_words;
    uint8_t active_mark_level; /* demodulated receiver usually 0: active-low */
    uint8_t discard_until_idle;
} gwir_amebad_rx_worker;

void gwir_amebad_rx_queue_init(gwir_amebad_rx_queue *q);
/* ISR-only producers. Do not call from two different IRQ contexts. */
int gwir_amebad_rx_irq_word(gwir_amebad_rx_queue *q, uint32_t raw_fifo_word);
int gwir_amebad_rx_irq_idle(gwir_amebad_rx_queue *q, uint32_t idle_us);
int gwir_amebad_rx_irq_error(gwir_amebad_rx_queue *q);
/* Returns 0 on success. sample_hz defines RX count tick frequency (e.g. 10 MHz).
 * An installed, hardware-specific board driver must supply real IR IRQ masking. */
int gwir_amebad_rx_worker_init(gwir_amebad_rx_worker *w, gwir_amebad_rx_queue *q,
                               gwir_engine *engine, gwir_amebad_rx_lock lock,
                               uint32_t sample_hz, uint8_t active_mark_level,
                               uint32_t carrier_guess_hz, uint16_t duty_guess_permille,
                               uint32_t gap_threshold_us);
/* Drains up to max_events in worker task. Positive: new complete captures;
 * 0: no complete frame; negative: invalid API input. Event-level failures
 * increment invalid_events or queue.overflows; never save partial captures.
 * Overrun is always a hard frame loss: discard to RX_IDLE resynchronization. */
int gwir_amebad_rx_worker_step(gwir_amebad_rx_worker *w, uint32_t max_events);
#endif
