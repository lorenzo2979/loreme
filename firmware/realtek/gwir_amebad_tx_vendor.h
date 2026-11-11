#ifndef GWIR_AMEBAD_TX_VENDOR_H
#define GWIR_AMEBAD_TX_VENDOR_H
#include "gwir_amebad_tx_fifo.h"
/* Requires an already initialized TX-mode IR peripheral. This interface
 * does not set pinmux, clocks, frequency, duty cycle or any RX state.
 * Only compile with the official AmebaD SDK or an API-compatible test shim.
 */
gwir_amebad_tx_ops gwir_amebad_vendor_tx_ops(void);
#endif
