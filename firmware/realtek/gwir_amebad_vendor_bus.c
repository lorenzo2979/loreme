#include "gwir_amebad_vendor_bus.h"
#include "gwir_amebad_tx_vendor.h"
#include "gwir_amebad_rx_irq.h"
#include "ameba_soc.h" /* Genuine AmebaD header on native CI, fake on host */
#include <stdint.h>

/* Realtek SDK AN0400: PA25 = IR TX, PA26 = IR RX.
 * Owner selected OpenBeken GPIO 25/26; electrical validation remains external.
 * Separate board integration must explicitly opt in; do not enable hardware
 * from a downloaded public dump, compile success, or flash image presence. */
#ifndef GWIR_GW006_IR_BENCH_OPT_IN
#define GWIR_GW006_IR_BENCH_OPT_IN 0
#endif
static unsigned board_pins_activated;
#if GWIR_GW006_IR_BENCH_OPT_IN == 1
static unsigned rx_irq_registered;
#endif
int gwir_amebad_vendor_board_ir_setup(void)
{
#if GWIR_GW006_IR_BENCH_OPT_IN == 1
    if (board_pins_activated || rx_irq_registered) return -2;
    Pinmux_Config(_PA_25, PINMUX_FUNCTION_IR);
    Pinmux_Config(_PA_26, PINMUX_FUNCTION_IR);
    board_pins_activated = 1u;
    return 0;
#else
    return -7;
#endif
}
static int board_verified(void *ctx)
{
    (void)ctx;
    return board_pins_activated == 1u;
}
static uint32_t vendor_now_ms(void *ctx) {
    (void)ctx;
    /* Task scheduler uptime; compiled but inaccessible without board approval.
       Tick conversion is tested by native build, not physical validation. */
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}
static int vendor_rx_start(void *ctx)
{
    (void)ctx;
#if GWIR_GW006_IR_BENCH_OPT_IN == 1
    if (!board_pins_activated || rx_irq_registered) return -1;
    IR_InitTypeDef cfg;
    IR_StructInit(&cfg);
    cfg.IR_Mode = IR_MODE_RX;
    cfg.IR_Freq = 10000000u;
    cfg.IR_RxFIFOThrLevel = 5u;
    cfg.IR_RxCntThrType = IR_RX_COUNT_LOW_LEVEL;
    cfg.IR_RxCntThr = 300000u; /* 30ms at 10MHz; test RX idle polarity */
    IR_Cmd(IR_DEV, IR_MODE_RX, DISABLE);
    IR_INTConfig(IR_DEV, IR_RX_INT_ALL_EN, DISABLE);
    IR_Init(IR_DEV, &cfg);
    IR_ClearRxFIFO(IR_DEV);
    /* Software IRQ queue is already bound by the IR pipeline. */
    InterruptRegister((IRQ_FUN)gwir_amebad_rx_irq_entry, IR_IRQ, 0u, 10);
    rx_irq_registered = 1u;
    IR_INTConfig(IR_DEV, IR_RX_INT_ALL_EN, ENABLE);
    InterruptEn(IR_IRQ, 10);
    IR_Cmd(IR_DEV, IR_MODE_RX, ENABLE);
    return 0;
#else
    return -1;
#endif
}
static int vendor_rx_stop(void *ctx)
{
    (void)ctx;
#if GWIR_GW006_IR_BENCH_OPT_IN == 1
    IR_Cmd(IR_DEV, IR_MODE_RX, DISABLE);
    IR_INTConfig(IR_DEV, IR_RX_INT_ALL_EN, DISABLE);
    if (rx_irq_registered) {
        InterruptDis(IR_IRQ);
        InterruptUnRegister(IR_IRQ);
        rx_irq_registered = 0u;
    }
    IR_ClearRxFIFO(IR_DEV);
    return 0;
#else
    return -1;
#endif
}
static int vendor_tx_prepare(void *ctx, uint32_t carrier, uint16_t duty) {
    (void)ctx;
    if (!board_pins_activated) return -1;
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
    ops.board_ir_ready = board_verified; /* requires explicit PA25/PA26 setup */
    ops.rx_start = vendor_rx_start;
    ops.rx_stop = vendor_rx_stop;
    ops.tx_prepare = vendor_tx_prepare;
    ops.tx_stop = vendor_tx_stop;
    ops.monotonic_ms = vendor_now_ms;
    ops.tx = gwir_amebad_vendor_tx_ops();
    return ops;
}
