#ifndef GWIR_AMEBAD_VENDOR_BUS_H
#define GWIR_AMEBAD_VENDOR_BUS_H
#include "gwir_amebad_bus.h"
/* Realtek IR TX PA25/RX PA26. Default build does not activate pinmux or IRQ.
 * GWIR_GW006_IR_BENCH_OPT_IN=1 alone does not initialize pins:
 * a board integration must explicitly call setup after checking wiring.
 * No flash authorization, key provisioning or boot registration is implied. */
int gwir_amebad_vendor_board_ir_setup(void);
gwir_amebad_bus_ops gwir_amebad_vendor_bus_ops(void);
#endif
