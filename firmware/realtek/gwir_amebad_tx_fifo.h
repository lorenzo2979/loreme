#ifndef GWIR_AMEBAD_TX_FIFO_H
#define GWIR_AMEBAD_TX_FIFO_H

#include <stddef.h>
#include <stdint.h>

/* The RTL8721D native IR peripheral has a 32-entry TX FIFO.
 * A single call to IR_SendBuf() MUST NOT exceed the currently free slots.
 * The final-data flag is set by IR_SendBuf() only for the very last word.
 * The device can still be actively transmitting after the last word is queued.
 */
#define GWIR_AMEBAD_TX_FIFO_DEPTH 32u

typedef struct {
    void *ctx;
    /* Number of currently free TX FIFO entries, between 0 and 32. */
    uint32_t (*free_slots)(void *ctx);
    /* Returns zero on success; final_chunk means END only on last word. */
    int (*write_words)(void *ctx, const uint32_t *words, uint32_t count, int final_chunk);
    /* Start TX after first chunk is written to FIFO. Exactly once per session. */
    int (*start_tx)(void *ctx);
    /* Returns 1 if TX FSM active, 0 if idle, negative on failure. */
    int (*is_running)(void *ctx);
} gwir_amebad_tx_ops;

typedef struct {
    const uint32_t *words; /* Caller must keep this array unchanged during TX. */
    uint32_t count;
    uint32_t next;
    uint8_t started;
    uint8_t last_queued;
    uint8_t completed;
    uint8_t failed;
} gwir_amebad_tx_session;

enum gwir_amebad_tx_step_result {
    GWIR_AMEBAD_TX_ERROR = -1,
    GWIR_AMEBAD_TX_WAIT = 0, /* More FIFO free space or TX draining needed */
    GWIR_AMEBAD_TX_PROGRESS = 1, /* Queued some words; not necessarily sent */
    GWIR_AMEBAD_TX_DONE = 2  /* TX idle and FIFO empty after final word */
};

/* Encoded words come from gwir_amebad_encode_tx(); this module does not
 * assign pins, configure peripherals, transmit a carrier or wait for gaps.
 * Caller must configure TX hardware, serialize access and enforce a timeout.
 */
int gwir_amebad_tx_begin(gwir_amebad_tx_session *s,
                        const uint32_t *words, uint32_t word_count);
int gwir_amebad_tx_step(gwir_amebad_tx_session *s,
                       const gwir_amebad_tx_ops *ops);

#endif
