#ifndef GWIR_AMEBAD_RX_VENDOR_H
#define GWIR_AMEBAD_RX_VENDOR_H
#include "gwir_amebad_rx_fifo.h"
/* IRQ-context API-shaped AmebaD bridge. Before calling, check the vendor RX
 * interrupt status and clear all pending IRQ bits. On counter-threshold IRQ,
 * first call drain(), then irq_idle(idle_us). On overflow/error call irq_error.
 * Does NOT configure GPIO/pinmux, register IRQ, change clocks or enable RX. */
int gwir_amebad_rx_vendor_irq_drain(gwir_amebad_rx_queue *queue);
#endif
