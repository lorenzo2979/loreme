/* SDK linkage diagnostic only: never enables IR, Wi-Fi, pins or TCP. */
#include "ameba_soc.h" /* must come from the Realtek SDK, not a shipped shim */
#include "../core/gwir_protocol.h"
#include "../core/gwir_engine.h"
#include "../core/gwir_auth.h"
#include "gwir_sdk_hmac.h"
#include "gwir_sdk_provisioned_identity.h"
#include "gwir_sdk_apconfig_port.h"
#include "gwir_sdk_apconfig_wifi.h"
#include "../core/gwir_apconfig_button.h"
#include "../core/gwir_apconfig_provision.h"
#include "../core/gwir_auth_dispatch.h"
#include "../realtek/gwir_amebad_tx_codec.h"
#include "../realtek/gwir_amebad_tx_fifo.h"
#include "../realtek/gwir_amebad_rx_fifo.h"
#include "../realtek/gwir_amebad_rx_irq.h"
#include "../realtek/gwir_amebad_bus.h"
#include "../realtek/gwir_amebad_vendor_bus.h"
#include "../realtek/gwir_amebad_board_pins.h"

void gwir_sdk_integration_banner(void)
{
    /* Checks linker reachability and the vendor's real IR struct/API. */
    IR_InitTypeDef ir_cfg;
    uint8_t msg[GWIR_HEADER_SIZE];
    uint8_t unused_digest[GWIR_AUTH_TAG_BYTES];
    size_t msg_bytes = 0u;
    const uint32_t pulses[2] = {560u, 560u};
    uint32_t tx_words[2];
    gwir_amebad_tx_session s;
    gwir_amebad_bus_ops native_bus_ops;
    int ok;

    /* IR_StructInit fills a local struct; it does not program the IR HW. */
    IR_StructInit(&ir_cfg);
    ok = (gwir_encode(msg, sizeof(msg), GWIR_PING, 1u, NULL, 0u,
                      &msg_bytes) == 0);
    ok = ok && msg_bytes == GWIR_HEADER_SIZE;
    ok = ok && (gwir_amebad_encode_tx(pulses, 2u, 38000u,
                                    tx_words, 2u) == 0);
    ok = ok && (gwir_amebad_tx_begin(&s, tx_words, 2u) == 0);
    /* Invalid arguments return without touching IR registers; this checks
       SDK linker reachability for the native RX interrupt bridge. */
    ok = ok && (gwir_amebad_rx_irq_service(NULL, 30000u, NULL) == -1);
    native_bus_ops = gwir_amebad_vendor_bus_ops();
    /* Verify new auth gate does not authorize an absent credential. */
    ok = ok && (gwir_auth_init(NULL, NULL, NULL, NULL, NULL) == GWIR_AUTH_REJECT);
    ok = ok && (gwir_auth_dispatch(NULL, NULL, NULL, NULL, 0u, NULL, 0u, NULL, 0u, NULL) == GWIR_AUTH_REJECT);
    /* Check the HMAC provider is linked but reject missing key before any crypto call. */
    ok = ok && (gwir_sdk_mbedtls_hmac(NULL, NULL, 32u, msg, sizeof(msg), unused_digest) == GWIR_SDK_HMAC_REJECT);
    /* Retain the real provider in the final linked image, while refusing
     * any attempt to generate credentials without a board-backed identity. */
    ok = ok && (gwir_sdk_identity_init(NULL, NULL, NULL) == -1);
    /* Verify fail-closed APConfig port, no GPIO, Wi-Fi or IR activity. */
    ok = ok && (gwir_apconfig_init(NULL, (gwir_apconfig_ops){0}) == -1);
    ok = ok && (gwir_sdk_button_read_pb23(NULL) == -1);
    ok = ok && (gwir_sdk_apconfig_start(NULL) == -1);
    ok = ok && (gwir_sdk_ap_wifi_enter(NULL, NULL) == -1);
    ok = ok && (gwir_provision_parse_form(NULL, 0u, NULL) == -1);
    /* Verify fail-closed physical-board gate, no IR register access. */
    ok = ok && native_bus_ops.board_ir_ready(native_bus_ops.ctx) == 0;
    DBG_8195A("[GWIR REV16] SDK link probe %s; e=%u,rxq=%u; IR/WiFi disabled\n",
              ok ? "OK" : "FAIL", (unsigned int)sizeof(gwir_engine),
              (unsigned int)sizeof(gwir_amebad_rx_queue));
}

/*
 * Real GW006 pre-scheduler registration hook. A separately validated board
 * port may provide a strong implementation of this weak, OPTIONAL function
 * returning a complete, static, authenticated configuration. A missing port
 * never creates credentials, never starts Wi-Fi or IR, and never writes flash.
 *
 * This is the missing path between the SDK main entry and the registered
 * GWIR-BOOT task. It does not certify an installable firmware by itself.
 */
#include "gwir_sdk_bootstrap.h"
__attribute__((weak))
const gwir_sdk_boot_config *gwir_gw006_board_port_config(void);

int gwir_sdk_board_prepare(void)
{
    const gwir_sdk_boot_config *cfg;
    /* Allow physical-button first-time APConfig even before a STA HMAC
     * identity exists. The SDK port refuses to create any task unless a
     * separately reviewed GW006 board approval and provisioning backend
     * are present. This call alone NEVER starts an open AP. */
    (void)gwir_sdk_apconfig_commission();
    if (!gwir_gw006_board_port_config) return -1;
    cfg = gwir_gw006_board_port_config();
    if (!cfg) return -1;
    /* The GW006 release must not start only a diagnostic Wi-Fi listener.
     * The product requires IR capture/playback AND persistent saved codes.
     * Actual partition ownership and key validity are checked separately. */
    if (!cfg->provider || !cfg->ir_pipeline || !cfg->ir_config ||
        !cfg->saved || !cfg->store || !cfg->store_io ||
        !cfg->store_io->read || !cfg->store_io->program ||
        !cfg->store_io->erase_sector || !cfg->pulse_scratch ||
        !cfg->upload_scratch) return -2;
    /* Validate the *real linked* GW006 pin profile before registering any
     * signed IR engine. This is only a logical pin-collision check; it does
     * not prove electrical separation of the red LED on PA25, and it does
     * NOT authorize the hardware, an IR IRQ, or any Flash access. */
    {
        gwir_board_pins pins = gwir_board_pins_candidate();
        if (gwir_board_pins_validate(&pins) != 0)
            return -3;
    }
    return gwir_sdk_boot_register(cfg);
}
