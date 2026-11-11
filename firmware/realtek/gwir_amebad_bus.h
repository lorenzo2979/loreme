#ifndef GWIR_AMEBAD_BUS_H
#define GWIR_AMEBAD_BUS_H
#include <stdint.h>
#include "gwir_amebad_tx_fifo.h"
#include "gwir_amebad_tx_codec.h"
#include "../core/gwir_protocol.h"

/* Portable, task-context-only arbitration for AmebaD's ONE half-duplex IR
 * controller.  An affirmative physical board authorization is mandatory.
 * Never call any of these functions from IR IRQ or parallel TCP tasks.
 */
typedef enum {
    GWIR_BUS_OFF = 0,
    GWIR_BUS_RX = 1,
    GWIR_BUS_TX = 2,
    GWIR_BUS_GAP = 3,
    GWIR_BUS_FAULT = 4
} gwir_amebad_bus_state;

typedef struct {
    void *ctx;
    /* Return 1 only if board's verified IR pinmux/IRQ wiring is installed.
       A zero/negative result MUST reject all hardware start operations. */
    int (*board_ir_ready)(void *ctx);
    int (*rx_start)(void *ctx);
    int (*rx_stop)(void *ctx);
    int (*tx_prepare)(void *ctx, uint32_t carrier_hz, uint16_t duty_permille);
    int (*tx_stop)(void *ctx);
    uint32_t (*monotonic_ms)(void *ctx); /* monotonic 32-bit milliseconds */
    gwir_amebad_tx_ops tx;
} gwir_amebad_bus_ops;

typedef struct {
    gwir_amebad_bus_ops ops;
    gwir_amebad_bus_state state;
    gwir_amebad_tx_session session;
    uint32_t tx_words[GWIR_MAX_PULSES_PER_FRAME]; /* ~8KB: allocate statically */
    uint32_t tx_started_ms;
    uint32_t gap_started_ms;
    uint32_t gap_ms;
    uint32_t timeout_ms;
    uint32_t errors;
    uint32_t tx_completed;
    uint8_t initialized;
} gwir_amebad_bus;

enum gwir_bus_result {
    GWIR_BUS_ERROR = -1,
    GWIR_BUS_REJECTED = -2,  /* board not approved, concurrent operation or bad args */
    GWIR_BUS_WAIT = 0,
    GWIR_BUS_PROGRESS = 1,
    GWIR_BUS_FINISHED = 2
};

int gwir_amebad_bus_init(gwir_amebad_bus *bus, const gwir_amebad_bus_ops *ops);
/* All transitions require a single serialized task. Never automatically
 * switch RX -> TX: capture is non-destructive until Android RX_ACK. */
int gwir_amebad_bus_start_rx(gwir_amebad_bus *bus);
int gwir_amebad_bus_stop_rx(gwir_amebad_bus *bus);
int gwir_amebad_bus_start_tx(gwir_amebad_bus *bus, const uint32_t *durations_us,
                            uint16_t count, uint32_t carrier_hz,
                            uint16_t duty_permille, uint32_t gap_after_us,
                            uint32_t timeout_ms);
/* Call frequently from the one bus task (never from IRQ). One bounded FIFO
 * step each call. TX completed only when hardware idle AND gap expired. */
int gwir_amebad_bus_poll(gwir_amebad_bus *bus);
/* Explicit recovery. No hardware starts until a new approved request. */
int gwir_amebad_bus_abort(gwir_amebad_bus *bus);
#endif
