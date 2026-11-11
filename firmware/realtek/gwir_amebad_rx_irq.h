#ifndef GWIR_AMEBAD_RX_IRQ_H
#define GWIR_AMEBAD_RX_IRQ_H
#include <stdint.h>
#include "gwir_amebad_rx_fifo.h"
/* No pinmux, clocks, interrupt enable or board flash.
 * Board code must validate pins before registering entry with IR_IRQ.
 * ISR only queues events; the task-level worker consumes them. */
typedef struct {
    uint32_t invocations;
    uint32_t fifo_words;
    uint32_t hardware_errors;
    uint32_t fifo_drops;
    uint32_t idle_events;
} gwir_amebad_rx_irq_stats;
int gwir_amebad_rx_irq_service(gwir_amebad_rx_queue *queue,
                               uint32_t idle_us,
                               gwir_amebad_rx_irq_stats *stats);
int gwir_amebad_rx_irq_bind(gwir_amebad_rx_queue *queue,
                            uint32_t idle_us,
                            gwir_amebad_rx_irq_stats *stats);
void gwir_amebad_rx_irq_unbind(void);
void gwir_amebad_rx_irq_entry(void);
#endif
