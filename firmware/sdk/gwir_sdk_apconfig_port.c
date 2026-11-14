/* GW006 APConfig runtime: physical-button-only, fail closed by default.
 * Board validation, GPIO wiring, DHCP and an AP-only provisioning listener
 * are NOT supplied by this file; no verified board port means NO AP.
 */
#include "ameba_soc.h"
#include "gwir_sdk_apconfig_port.h"
#include "gwir_sdk_apconfig_wifi.h"
#include "../core/gwir_apconfig_controller.h"
#include "../core/gwir_apconfig_tcp_transport.h"
#include <stddef.h>
#include <string.h>

#define GWIR_AP_POLL_MS 50u
/* Only a separately reviewed GW006 board port may supply these strong hooks.
 * A weak, absent hook NEVER permits setup mode or touches any GPIO. */
__attribute__((weak)) int gwir_gw006_apconfig_board_approved(void);
__attribute__((weak)) const gwir_apcfg_backend *gwir_gw006_apconfig_board_backend(void);
/* Realtek board hooks MUST attest listener + accepted socket as belonging
 * to the actual setup AP netif. Comparing client IP or Host is insufficient.
 * The station backend MUST stage, verify and atomically commit settings. */
__attribute__((weak)) const gwir_aptcp_ops *gwir_gw006_apconfig_board_tcp_ops(void);
__attribute__((weak)) const gwir_station_ops *gwir_gw006_apconfig_board_station_ops(void);

static gwir_apconfig_controller controller;
static gwir_apcfg_backend backend;
static unsigned backend_installed, controller_initialized, task_started;
static const gwir_aptcp_ops *board_tcp;
static const gwir_station_ops *board_station;
static unsigned settings_committed;

/* No callback is allowed to write directly to active Wi-Fi credentials.
 * Delegate all validation, staging and rollback to the vetted board port. */
static int station_stage(void *ctx, const gwir_station_profile *p)
{
    (void)ctx;
    return board_station->stage(board_station->ctx, p);
}
static int station_verify(void *ctx)
{
    (void)ctx;
    return board_station->verify(board_station->ctx);
}
static int station_commit(void *ctx)
{
    (void)ctx;
    if (board_station->commit(board_station->ctx) != 0) return -1;
    settings_committed = 1u; /* close AP after HTTP reply, not during send */
    return 0;
}
static void station_rollback(void *ctx)
{
    (void)ctx;
    board_station->rollback(board_station->ctx);
}
static const gwir_station_ops station_proxy = {
    NULL, station_stage, station_verify, station_commit, station_rollback
};
static int valid_tcp_ops(const gwir_aptcp_ops *o)
{
    return o && o->listener_on_ap && o->accept_one && o->client_on_ap &&
           o->recv_some && o->send_some && o->close_client && o->now_ms;
}
static int valid_station_ops(const gwir_station_ops *o)
{
    return o && o->stage && o->verify && o->commit && o->rollback;
}

static int board_approved(void)
{
    return gwir_gw006_apconfig_board_approved &&
           gwir_gw006_apconfig_board_approved() == 1;
}
int gwir_sdk_button_read_pb23(int *pressed)
{
    if (!pressed) return -1;
    *pressed = 0;
    if (!board_approved()) return -2;
    /* Board port is responsible for testing PB23 polarity and pull-up
       before returning approval. No pinmux writes occur here. */
    *pressed = (GPIO_ReadDataBit(_PB_23) == 0u) ? 1 : 0;
    return 0;
}
static int read_button(void *ctx, int *pressed)
{
    (void)ctx;
    return gwir_sdk_button_read_pb23(pressed);
}
static int fixed_public_identity(void *ctx, gwir_apcfg_identity *out)
{
    (void)ctx;
    if (!out || !backend.load_identity ||
        backend.load_identity(backend.ctx, out) != 0) return -1;
    /* The temporary network's name must be exactly the user-approved SSID.
       Reject other names; never fall back to a permanent/open default. */
    return memcmp(out->ssid, "GW006-IR-SETUP", sizeof("GW006-IR-SETUP")) == 0 ? 0 : -1;
}
int gwir_sdk_apconfig_install_backend(gwir_apcfg_backend ops)
{
    if (backend_installed || task_started ||
        ops.wifi_enter != gwir_sdk_ap_wifi_enter ||
        ops.wifi_leave != gwir_sdk_ap_wifi_leave ||
        !ops.load_identity || !ops.provisioner_start || !ops.provisioner_stop)
        return -1;
    backend = ops;
    backend_installed = 1u;
    return 0;
}
static void apconfig_task(void *unused)
{
    (void)unused;
    /* No board approval: only cleanup; never reopen an unaudited radio. */
    for (;;) {
        int rc;
        if (!board_approved()) {
            if (gwir_apconfig_controller_stop(&controller) == 0) break;
        } else {
            rc = gwir_apconfig_controller_poll(&controller,
                 (uint32_t)xTaskGetTickCount() * (uint32_t)portTICK_PERIOD_MS);
            if (rc < 0) (void)gwir_apconfig_controller_stop(&controller);
            else if (gwir_apcfg_session_is_active(&controller.session) &&
                     board_tcp && board_station) {
                /* One session task owns the HTTP parser AND button polling.
                 * The socket adapter MUST enforce timeouts and AP netif
                 * affinity; no public LAN listener is created here. */
                int served = gwir_aptcp_serve_one(board_tcp,
                                    &controller.session, &station_proxy);
                if (settings_committed) {
                    settings_committed = 0u;
                    (void)gwir_apconfig_controller_committed(&controller);
                } else if (served == GWIR_APTCP_IO_FAILURE ||
                           served == GWIR_APTCP_UNAVAILABLE) {
                    (void)gwir_apconfig_controller_stop(&controller);
                }
            }
        }
        vTaskDelay((GWIR_AP_POLL_MS + portTICK_PERIOD_MS - 1u) /
                   portTICK_PERIOD_MS);
    }
    task_started = 0u;
    vTaskDelete(NULL);
}
int gwir_sdk_apconfig_commission(void)
{
    const gwir_apcfg_backend *ops;
    gwir_apcfg_backend safe;
    if (task_started || !board_approved() ||
        !gwir_gw006_apconfig_board_backend ||
        !gwir_gw006_apconfig_board_tcp_ops ||
        !gwir_gw006_apconfig_board_station_ops ||
        portTICK_PERIOD_MS == 0u) return -1;
    /* Fail before touching Wi-Fi when the AP-only socket/rollback backend
       does not exist. This also prevents an AP with no functional portal. */
    board_tcp = gwir_gw006_apconfig_board_tcp_ops();
    board_station = gwir_gw006_apconfig_board_station_ops();
    if (!valid_tcp_ops(board_tcp) || !valid_station_ops(board_station))
        return -2;
    if (!backend_installed) {
        ops = gwir_gw006_apconfig_board_backend();
        if (!ops || gwir_sdk_apconfig_install_backend(*ops) != 0) return -2;
    }
    if (!controller_initialized) {
        safe = backend;
        safe.load_identity = fixed_public_identity;
        if (gwir_apconfig_controller_init(&controller, safe, read_button, NULL) != 0)
            return -3;
        controller_initialized = 1u;
    }
    /* Pre-scheduler task creation. Only a physical 5s press can start AP.
       Starting this task does NOT activate Wi-Fi, GPIO output or flash. */
    if (xTaskCreate(apconfig_task, "GWIR-APCFG", 1024u, NULL,
                    tskIDLE_PRIORITY + 1u, NULL) != pdPASS)
        return -4;
    task_started = 1u;
    return 0;
}
/* Old programmatic AP start is deliberately disabled: physical press only. */
int gwir_sdk_apconfig_start(void *ctx)
{
    (void)ctx;
    return -1;
}
/* Stop is allowed even when approval is revoked, to close the open AP. */
int gwir_sdk_apconfig_stop(void *ctx)
{
    (void)ctx;
    if (!controller_initialized) return -1;
    return gwir_apconfig_controller_stop(&controller);
}
int gwir_sdk_apconfig_committed(void)
{
    if (!controller_initialized) return -1;
    return gwir_apconfig_controller_committed(&controller);
}
