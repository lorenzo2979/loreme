#include "gwir_amebad_tx_fifo.h"
#include "../core/gwir_protocol.h"

int gwir_amebad_tx_begin(gwir_amebad_tx_session *s,
                        const uint32_t *words, uint32_t word_count)
{
    if (!s || !words || word_count == 0u ||
        word_count > GWIR_MAX_PULSES_PER_FRAME) return -1;
    s->words = words;
    s->count = word_count;
    s->next = 0u;
    s->started = 0u;
    s->last_queued = 0u;
    s->completed = 0u;
    s->failed = 0u;
    return 0;
}

int gwir_amebad_tx_step(gwir_amebad_tx_session *s,
                       const gwir_amebad_tx_ops *ops)
{
    if (!s || !ops || !ops->free_slots || !ops->write_words ||
        !ops->start_tx || !ops->is_running || !s->words ||
        !s->count || s->next > s->count || s->failed) {
        return GWIR_AMEBAD_TX_ERROR;
    }
    if (s->completed) return GWIR_AMEBAD_TX_DONE;

    /* A 32-word FIFO requires repeated refills for full RAW AC codes. */
    if (!s->last_queued) {
        const uint32_t free = ops->free_slots(ops->ctx);
        if (free > GWIR_AMEBAD_TX_FIFO_DEPTH) {
            s->failed = 1u;
            return GWIR_AMEBAD_TX_ERROR;
        }
        if (free == 0u) return GWIR_AMEBAD_TX_WAIT;
        const uint32_t remaining = s->count - s->next;
        if (!remaining) {
            s->failed = 1u;
            return GWIR_AMEBAD_TX_ERROR;
        }
        const uint32_t take = remaining < free ? remaining : free;
        const int is_last = take == remaining;
        if (ops->write_words(ops->ctx, s->words + s->next, take, is_last)) {
            s->failed = 1u;
            return GWIR_AMEBAD_TX_ERROR;
        }
        s->next += take;
        if (is_last) s->last_queued = 1u;
        if (!s->started) {
            /* Start only after the initial FIFO payload has been accepted. */
            if (ops->start_tx(ops->ctx)) {
                s->failed = 1u;
                return GWIR_AMEBAD_TX_ERROR;
            }
            s->started = 1u;
        }
        return GWIR_AMEBAD_TX_PROGRESS;
    }

    /* End-marker queued != IR finished. Check both FSM and FIFO. */
    const int running = ops->is_running(ops->ctx);
    if (running < 0) {
        s->failed = 1u;
        return GWIR_AMEBAD_TX_ERROR;
    }
    if (running || ops->free_slots(ops->ctx) != GWIR_AMEBAD_TX_FIFO_DEPTH)
        return GWIR_AMEBAD_TX_WAIT;
    s->completed = 1u;
    return GWIR_AMEBAD_TX_DONE;
}
