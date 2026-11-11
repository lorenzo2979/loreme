#include "ameba_soc.h"
#include "gwir_sdk_apconfig_port.h"
#include "gwir_sdk_apconfig_wifi.h"
#include <stddef.h>

/* The physical GW006 pin routing and recovery process are NOT verified.
   No public setter: cannot enable button/AP at runtime in REV15. */
static volatile unsigned board_button_approved_flag = 0u;
static int board_button_approved(void) { return board_button_approved_flag != 0u; }
static gwir_apcfg_session ap_session;
static int backend_installed;

int gwir_sdk_button_read_pb23(int *pressed)
{
    if (!pressed) return -1;
    *pressed = 0;
    if (!board_button_approved()) return -2;
    /* PB23 is observed in OpenBeken, but hardware pin/polarity must be tested.
       This path is unreachable until a board-specific, audited REV is made. */
    *pressed = (GPIO_ReadDataBit(_PB_23) == 0u) ? 1 : 0;
    return 0;
}

int gwir_sdk_apconfig_install_backend(gwir_apcfg_backend ops)
{
    /* Accept only the designated OPEN-AP transport. The provisioner must
       bind to AP only; no production-LAN configuration endpoint. */
    if (ops.wifi_enter != gwir_sdk_ap_wifi_enter ||
        ops.wifi_leave != gwir_sdk_ap_wifi_leave ||
        !ops.load_identity || !ops.provisioner_start || !ops.provisioner_stop)
        return -1;
    if (backend_installed) return -2;
    if (gwir_apcfg_session_init(&ap_session, ops) != 0) return -3;
    backend_installed = 1;
    return 0;
}
int gwir_sdk_apconfig_start(void *ctx)
{
    (void)ctx;
    if (!board_button_approved() || !backend_installed) return -1;
    return gwir_apcfg_session_start(&ap_session);
}
int gwir_sdk_apconfig_stop(void *ctx)
{
    (void)ctx;
    if (!backend_installed || !board_button_approved()) return -1;
    return gwir_apcfg_session_stop(&ap_session);
}
