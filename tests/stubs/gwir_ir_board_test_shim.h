#ifndef GWIR_TEST_AMED_IR_BOARD_SHIM_H
#define GWIR_TEST_AMED_IR_BOARD_SHIM_H
/* Host test only: never copied into the native Realtek SDK. */
#include "ameba_soc.h"
#define PINMUX_FUNCTION_IR 0x14u
#define IR_RX_INT_ALL_EN 0x3fu
#define IR_IRQ 57u
typedef void (*IRQ_FUN)(void);
void Pinmux_Config(u32 pin, u32 function);
void IR_INTConfig(IR_TypeDef *ir, u32 mask, u32 state);
void InterruptRegister(IRQ_FUN handler, u32 irq, u32 user, u32 priority);
void InterruptEn(u32 irq, u32 priority);
void InterruptDis(u32 irq);
void InterruptUnRegister(u32 irq);
#endif
