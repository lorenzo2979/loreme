#include "gwir_amebad_rx_vendor.h"
#include "ameba_soc.h" /* Official SDK header required for an actual device build */

int gwir_amebad_rx_vendor_irq_drain(gwir_amebad_rx_queue *q) {
    if (!q) return -1;
    /* The hardware FIFO depth is 32. The loop is bounded to prevent a
     * misconfigured IRQ from spinning indefinitely on the chip. */
    uint32_t handled=0;
    int dropped=0;
    while (handled<32u) {
        uint32_t n=IR_GetRxDataLen(IR_DEV);
        if (n==0u) break;
        if (n>32u) return -1;
        uint32_t take=n;
        if (take>32u-handled) take=32u-handled;
        for(uint32_t i=0;i<take;i++) {
            uint32_t raw=IR_ReceiveData(IR_DEV);
            if (gwir_amebad_rx_irq_word(q,raw)) dropped=1;
        }
        handled+=take;
    }
    return dropped?-2:(int)handled;
}
