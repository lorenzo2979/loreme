#ifndef GWIR_AMEBAD_VENDOR_BUS_H
#define GWIR_AMEBAD_VENDOR_BUS_H
#include "gwir_amebad_bus.h"
/* SDK-backed API, but deliberately FAIL-CLOSED: board_ir_ready returns 0
 * until the board-specific pinmux, IRQ handler and electrical interface are
 * verified. This source adds NO interrupts or pinmux changes on startup. */
gwir_amebad_bus_ops gwir_amebad_vendor_bus_ops(void);
#endif
