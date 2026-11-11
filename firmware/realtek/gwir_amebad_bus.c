#include "gwir_amebad_bus.h"
#include <limits.h>

static void inc_errors(gwir_amebad_bus *b) {
    if (b->errors < UINT32_MAX) ++b->errors;
}
static int valid(const gwir_amebad_bus *b) {
    return b && b->initialized && b->ops.board_ir_ready &&
           b->ops.monotonic_ms && b->ops.tx_prepare && b->ops.tx_stop &&
           b->ops.rx_start && b->ops.rx_stop;
}
static int gate(gwir_amebad_bus *b) {
    return valid(b) && b->ops.board_ir_ready(b->ops.ctx) == 1;
}
int gwir_amebad_bus_init(gwir_amebad_bus *b, const gwir_amebad_bus_ops *ops) {
    if (!b || !ops || !ops->board_ir_ready || !ops->rx_start ||
        !ops->rx_stop || !ops->tx_prepare || !ops->tx_stop ||
        !ops->monotonic_ms || !ops->tx.free_slots ||
        !ops->tx.write_words || !ops->tx.start_tx || !ops->tx.is_running)
        return GWIR_BUS_REJECTED;
    /* No huge stack temporary: this object contains an 8KB FIFO buffer. */
    b->ops = *ops;
    b->state = GWIR_BUS_OFF;
    b->session = (gwir_amebad_tx_session){0};
    b->tx_started_ms = 0u;
    b->gap_started_ms = 0u;
    b->gap_ms = 0u;
    b->timeout_ms = 0u;
    b->errors = 0u;
    b->tx_completed = 0u;
    b->initialized = 1u;
    return 0;
}
int gwir_amebad_bus_start_rx(gwir_amebad_bus *b) {
    if (!valid(b) || b->state != GWIR_BUS_OFF || !gate(b)) return GWIR_BUS_REJECTED;
    if (b->ops.rx_start(b->ops.ctx)) {
        /* RX config might have partially altered hardware. Try to stop it. */
        if (b->ops.rx_stop(b->ops.ctx)) b->state = GWIR_BUS_FAULT;
        inc_errors(b);
        return GWIR_BUS_ERROR;
    }
    b->state = GWIR_BUS_RX;
    return 0;
}
int gwir_amebad_bus_stop_rx(gwir_amebad_bus *b) {
    if (!valid(b) || b->state != GWIR_BUS_RX) return GWIR_BUS_REJECTED;
    if (b->ops.rx_stop(b->ops.ctx)) {
        b->state = GWIR_BUS_FAULT;
        inc_errors(b);
        return GWIR_BUS_ERROR;
    }
    b->state = GWIR_BUS_OFF;
    return 0;
}
int gwir_amebad_bus_start_tx(gwir_amebad_bus *b, const uint32_t *duration,
                            uint16_t n, uint32_t carrier_hz,
                            uint16_t duty, uint32_t gap_us,
                            uint32_t timeout_ms) {
    if (!valid(b) || b->state != GWIR_BUS_OFF || !gate(b) ||
        !duration || !n || n > GWIR_MAX_PULSES_PER_FRAME ||
        gap_us > 30000000u || timeout_ms < 10u || timeout_ms > 120000u ||
        (gap_us + 999u)/1000u > timeout_ms)
        return GWIR_BUS_REJECTED;
    /* Validate+encode before any register access. Ensure no partial TX. */
    if (gwir_amebad_encode_tx(duration, n, carrier_hz, b->tx_words,
                              GWIR_MAX_PULSES_PER_FRAME))
        return GWIR_BUS_REJECTED;
    if (gwir_amebad_tx_begin(&b->session, b->tx_words, n))
        return GWIR_BUS_REJECTED;
    if (b->ops.tx_prepare(b->ops.ctx, carrier_hz, duty)) {
        if (b->ops.tx_stop(b->ops.ctx)) b->state = GWIR_BUS_FAULT;
        inc_errors(b);
        return GWIR_BUS_ERROR;
    }
    b->tx_started_ms = b->ops.monotonic_ms(b->ops.ctx);
    b->gap_ms = (gap_us + 999u)/1000u;
    b->timeout_ms = timeout_ms;
    b->state = GWIR_BUS_TX;
    return 0;
}
int gwir_amebad_bus_abort(gwir_amebad_bus *b) {
    if (!valid(b)) return GWIR_BUS_REJECTED;
    int r = 0;
    if (b->state == GWIR_BUS_RX || b->state == GWIR_BUS_FAULT)
        r |= b->ops.rx_stop(b->ops.ctx);
    if (b->state == GWIR_BUS_TX || b->state == GWIR_BUS_GAP ||
        b->state == GWIR_BUS_FAULT)
        r |= b->ops.tx_stop(b->ops.ctx);
    if (r) {
        b->state = GWIR_BUS_FAULT;
        inc_errors(b);
        return GWIR_BUS_ERROR;
    }
    b->state = GWIR_BUS_OFF;
    return 0;
}
int gwir_amebad_bus_poll(gwir_amebad_bus *b) {
    if (!valid(b)) return GWIR_BUS_REJECTED;
    if (b->state == GWIR_BUS_FAULT) return GWIR_BUS_ERROR;
    if (b->state == GWIR_BUS_OFF) return GWIR_BUS_FINISHED;
    if (b->state == GWIR_BUS_RX) return GWIR_BUS_WAIT;
    uint32_t now = b->ops.monotonic_ms(b->ops.ctx);
    /* uint32 subtraction remains correct across one wrap, with timeouts < 2^31. */
    if ((uint32_t)(now - b->tx_started_ms) > b->timeout_ms) {
        (void)gwir_amebad_bus_abort(b);
        inc_errors(b);
        return GWIR_BUS_ERROR;
    }
    if (b->state == GWIR_BUS_GAP) {
        if ((uint32_t)(now - b->gap_started_ms) < b->gap_ms)
            return GWIR_BUS_WAIT;
        b->state = GWIR_BUS_OFF;
        if (b->tx_completed < UINT32_MAX) ++b->tx_completed;
        return GWIR_BUS_FINISHED;
    }
    int r = gwir_amebad_tx_step(&b->session, &b->ops.tx);
    if (r == GWIR_AMEBAD_TX_ERROR) {
        (void)gwir_amebad_bus_abort(b);
        inc_errors(b);
        return GWIR_BUS_ERROR;
    }
    if (r == GWIR_AMEBAD_TX_DONE) {
        if (b->ops.tx_stop(b->ops.ctx)) {
            b->state = GWIR_BUS_FAULT;
            inc_errors(b);
            return GWIR_BUS_ERROR;
        }
        b->gap_started_ms = now;
        b->state = GWIR_BUS_GAP;
        if (!b->gap_ms) {
            b->state = GWIR_BUS_OFF;
            if (b->tx_completed < UINT32_MAX) ++b->tx_completed;
            return GWIR_BUS_FINISHED;
        }
        return GWIR_BUS_WAIT;
    }
    return r == GWIR_AMEBAD_TX_PROGRESS ? GWIR_BUS_PROGRESS : GWIR_BUS_WAIT;
}
