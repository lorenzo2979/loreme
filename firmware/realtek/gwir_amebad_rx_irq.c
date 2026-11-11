#include "gwir_amebad_rx_irq.h"
#include "gwir_amebad_rx_vendor.h"
#include "ameba_soc.h"
#include <stddef.h>

/* Never enables pins, clocks or IRQ: registration belongs to verified board code. */
static gwir_amebad_rx_queue *bound_queue;
static gwir_amebad_rx_irq_stats *bound_stats;
static uint32_t bound_idle_us;

static void count_inc(uint32_t *p) { if (*p < UINT32_MAX) ++*p; }

int gwir_amebad_rx_irq_service(gwir_amebad_rx_queue *queue,
                               uint32_t idle_us,
                               gwir_amebad_rx_irq_stats *stats)
{
    if (!queue || idle_us < 1000u || idle_us > 30000000u) return -1;
    const u32 status = IR_GetINTStatus(IR_DEV);
    if (status == 0u) return 0;
    const u32 old_mask = IR_GetIMR(IR_DEV);
    IR_MaskINTConfig(IR_DEV, IR_RX_INT_ALL_MASK, DISABLE);
    if (stats) count_inc(&stats->invocations);

    const u32 fault = status & (IR_RX_FIFO_OF_INT_STATUS |
        IR_RX_FIFO_FULL_INT_STATUS | IR_RX_CNT_OF_INT_STATUS |
        IR_RX_FIFO_ERROR_INT_STATUS);
    int drained = 0;
    if (fault) {
        /* Discard hardware remainder, not a truncated learned frame. */
        (void)gwir_amebad_rx_irq_error(queue);
        IR_ClearRxFIFO(IR_DEV);
        if (stats) count_inc(&stats->hardware_errors);
        drained = -2;
    } else {
        /* Also drain below-threshold FIFO contents on counter idle IRQ. */
        drained = gwir_amebad_rx_vendor_irq_drain(queue);
        if (drained < 0) {
            (void)gwir_amebad_rx_irq_error(queue);
            if (stats) count_inc(&stats->fifo_drops);
        } else if (stats) {
            uint32_t remaining = UINT32_MAX - stats->fifo_words;
            stats->fifo_words += (uint32_t)drained > remaining ? remaining : (uint32_t)drained;
        }
    }

    if (status & IR_RX_CNT_THR_INT_STATUS) {
        if (gwir_amebad_rx_irq_idle(queue, idle_us) < 0) {
            if (stats) count_inc(&stats->fifo_drops);
        } else if (stats) count_inc(&stats->idle_events);
    }
    if (status & IR_RX_FIFO_FULL_INT_STATUS)
        IR_ClearINTPendingBit(IR_DEV, IR_RX_FIFO_FULL_INT_CLR);
    if (status & IR_RX_FIFO_LEVEL_INT_STATUS)
        IR_ClearINTPendingBit(IR_DEV, IR_RX_FIFO_LEVEL_INT_CLR);
    if (status & IR_RX_CNT_OF_INT_STATUS)
        IR_ClearINTPendingBit(IR_DEV, IR_RX_CNT_OF_INT_CLR);
    if (status & IR_RX_FIFO_OF_INT_STATUS)
        IR_ClearINTPendingBit(IR_DEV, IR_RX_FIFO_OF_INT_CLR);
    if (status & IR_RX_CNT_THR_INT_STATUS)
        IR_ClearINTPendingBit(IR_DEV, IR_RX_CNT_THR_INT_CLR);
    if (status & IR_RX_FIFO_ERROR_INT_STATUS)
        IR_ClearINTPendingBit(IR_DEV, IR_RX_FIFO_ERROR_INT_CLR);

    IR_MaskINTConfig(IR_DEV, old_mask, ENABLE);
    return drained;
}

int gwir_amebad_rx_irq_bind(gwir_amebad_rx_queue *queue,
                            uint32_t idle_us,
                            gwir_amebad_rx_irq_stats *stats)
{
    if (!queue || idle_us < 1000u || idle_us > 30000000u) return -1;
    if (bound_queue && bound_queue != queue) return -2;
    bound_queue = queue;
    bound_idle_us = idle_us;
    bound_stats = stats;
    return 0;
}
void gwir_amebad_rx_irq_unbind(void)
{
    bound_queue = NULL;
    bound_idle_us = 0u;
    bound_stats = NULL;
}
void gwir_amebad_rx_irq_entry(void)
{
    if (bound_queue) (void)gwir_amebad_rx_irq_service(bound_queue,
                                                      bound_idle_us, bound_stats);
}
