#include "gwir_amebad_vendor_bus.h"
#include "gwir_amebad_tx_vendor.h"
#include "ameba_soc.h" /* Genuine AmebaD header on native CI, fake on host */
#include <stdint.h>

/* Never substitute generic module UART pins for an unverified IR route.
 * Do not set this to true merely to produce a .bin. */
static int board_verified(void *ctx) {
    (void)ctx;
    return 0;
}
static uint32_t vendor_now_ms(void *ctx) {
    (void)ctx;
    /* Task scheduler uptime; compiled but inaccessible without board approval.
       Tick conversion is tested by native build, not physical validation. */
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}
static int vendor_rx_start(void *ctx) {
    (void)ctx;
    /* This configuration is ONLY reachable if a future board-specific
       implementation explicitly validates the pins and configures IR IRQ.
       RX polarity/frequency must be measured on the real circuit. */
    IR_InitTypeDef cfg;
    IR_StructInit(&cfg);
    cfg.IR_Mode = IR_MODE_RX;
    cfg.IR_Freq = 10000000u;
    cfg.IR_RxFIFOThrLevel = 5u;
    cfg.IR_RxCntThrType = IR_RX_COUNT_LOW_LEVEL;
    cfg.IR_RxCntThr = 300000u; /* 30 ms at 10 MHz; board-specific assumption */
    IR_Cmd(IR_DEV, IR_MODE_RX, DISABLE);
    IR_Init(IR_DEV, &cfg);
    IR_ClearRxFIFO(IR_DEV);
    IR_Cmd(IR_DEV, IR_MODE_RX, ENABLE);
    return 0;
}
static int vendor_rx_stop(void *ctx) {
    (void)ctx;
    IR_Cmd(IR_DEV, IR_MODE_RX, DISABLE);
    IR_ClearRxFIFO(IR_DEV);
    return 0;
}
static int vendor_tx_prepare(void *ctx, uint32_t carrier, uint16_t duty) {
    (void)ctx;
    if (carrier < 25000u || carrier > 500000u || duty < 200u || duty > 500u)
        return -1;
    /* RTL8721D duty is an integer divider, not exact permille. Reject if
       requested duty is farther than 25 permille from achievable value. */
    uint32_t divider = (1000u + duty/2u) / duty;
    if (divider < 2u || divider > 5u) return -1;
    uint32_t actual = 1000u / divider;
    uint32_t error = actual > duty ? actual - duty : duty - actual;
    if (error > 25u) return -1;
    IR_InitTypeDef cfg;
    IR_StructInit(&cfg);
    cfg.IR_Mode = IR_MODE_TX;
    cfg.IR_Freq = carrier;
    cfg.IR_DutyCycle = divider;
    IR_Cmd(IR_DEV, IR_MODE_TX, DISABLE);
    IR_Init(IR_DEV, &cfg);
    IR_ClearTxFIFO(IR_DEV);
    return 0;
}
static int vendor_tx_stop(void *ctx) {
    (void)ctx;
    IR_Cmd(IR_DEV, IR_MODE_TX, DISABLE);
    IR_ClearTxFIFO(IR_DEV);
    return 0;
}
gwir_amebad_bus_ops gwir_amebad_vendor_bus_ops(void) {
    gwir_amebad_bus_ops ops;
    ops.ctx = NULL;
    ops.board_ir_ready = board_verified; /* always false in REV11 */
    ops.rx_start = vendor_rx_start;
    ops.rx_stop = vendor_rx_stop;
    ops.tx_prepare = vendor_tx_prepare;
    ops.tx_stop = vendor_tx_stop;
    ops.monotonic_ms = vendor_now_ms;
    ops.tx = gwir_amebad_vendor_tx_ops();
    return ops;
}
