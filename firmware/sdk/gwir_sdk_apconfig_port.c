#include "ameba_soc.h"
#include "gwir_sdk_apconfig_port.h"
#include <stddef.h>

/* PB23 was observed as button in OpenBeken, but the real physical input
 * polarity, voltage, pull resistor and competing IR pinmux must be audited.
 * No Pinmux_Config, GPIO_Init, AP start or credential operations here. */
static volatile unsigned board_button_approved_flag = 0u;
static int board_button_approved(void) { return board_button_approved_flag != 0u; }

int gwir_sdk_button_read_pb23(int *pressed)
{
    if (!pressed) return -1;
    *pressed = 0;
    if (!board_button_approved()) return -2;
    /* GPIO read is compiled against the vendor SDK but UNREACHABLE until
       physical approval. Active-low interpretation is a hypothesis. */
    *pressed = (GPIO_ReadDataBit(_PB_23) == 0u) ? 1 : 0;
    return 0;
}

/* A real AP service MUST enforce unique WPA2+ access credentials, physical
 * initiation, 5-minute expiration, and authenticated configuration handling.
 * Until implemented and tested, the callback refuses to open a hotspot. */
int gwir_sdk_apconfig_start(void *ctx) { (void)ctx; return -1; }
int gwir_sdk_apconfig_stop(void *ctx) { (void)ctx; return -1; }
