#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/realtek/gwir_amebad_vendor_bus.h"
#include "ameba_soc.h"
static unsigned pin_count, rx_on, irq_registered, irq_enabled, irq_disabled, irq_unregistered;
static u32 pins[2];
IR_TypeDef gwir_ir_test_device;
u32 xTaskGetTickCount(void) { return 9u; }
void Pinmux_Config(u32 pin, u32 func) { assert(func==PINMUX_FUNCTION_IR && pin_count<2u); pins[pin_count++]=pin; }
void IR_StructInit(IR_InitTypeDef *c) { memset(c,0,sizeof(*c)); }
void IR_Init(IR_TypeDef *ir,IR_InitTypeDef *c) {(void)ir;(void)c;}
void IR_ClearTxFIFO(IR_TypeDef *ir) {(void)ir;}
void IR_ClearRxFIFO(IR_TypeDef *ir) {(void)ir;}
void IR_Cmd(IR_TypeDef *ir,u32 mode,u32 state) {(void)ir;(void)mode;(void)state;}
void IR_INTConfig(IR_TypeDef *ir,u32 mask,u32 state) {
    (void)ir; assert(mask==IR_RX_INT_ALL_EN); rx_on+=(state==ENABLE);
}
void InterruptRegister(IRQ_FUN handler,u32 irq,u32 user,u32 priority) {
    assert(handler && irq==IR_IRQ && user==0u && priority==10u); ++irq_registered;
}
void InterruptEn(u32 irq,u32 priority) {assert(irq==IR_IRQ && priority==10u);++irq_enabled;}
void InterruptDis(u32 irq) {assert(irq==IR_IRQ);++irq_disabled;}
void InterruptUnRegister(u32 irq) {assert(irq==IR_IRQ);++irq_unregistered;}
void gwir_amebad_rx_irq_entry(void) {}
gwir_amebad_tx_ops gwir_amebad_vendor_tx_ops(void) {
    gwir_amebad_tx_ops ops={0}; return ops;
}
int main(void) {
    gwir_amebad_bus_ops ops=gwir_amebad_vendor_bus_ops();
    assert(ops.board_ir_ready(ops.ctx)==0);
#if GWIR_GW006_IR_BENCH_OPT_IN == 1
    assert(gwir_amebad_vendor_board_ir_setup()==0);
    assert(pin_count==2u && pins[0]==_PA_25 && pins[1]==_PA_26);
    assert(gwir_amebad_vendor_board_ir_setup()!=0);
    assert(ops.board_ir_ready(ops.ctx)==1);
    assert(ops.rx_start(ops.ctx)==0);
    assert(irq_registered==1u && irq_enabled==1u && rx_on==1u);
    assert(ops.rx_start(ops.ctx)!=0);
    assert(ops.rx_stop(ops.ctx)==0);
    assert(irq_disabled==1u && irq_unregistered==1u);
    assert(ops.rx_start(ops.ctx)==0);
    assert(irq_enabled==2u);
    assert(ops.rx_stop(ops.ctx)==0);
    assert(irq_disabled==2u && irq_unregistered==2u);
#else
    assert(gwir_amebad_vendor_board_ir_setup()==-7);
    assert(pin_count==0u && irq_registered==0u && irq_enabled==0u);
    assert(ops.board_ir_ready(ops.ctx)==0);
    assert(ops.rx_start(ops.ctx)!=0);
#endif
    puts("PASS: Realtek GW006 IR GPIO PA25/PA26, hardware IRQ and default deny");
    return 0;
}
