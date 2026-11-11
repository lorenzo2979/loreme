/* SDK linkage diagnostic only: never enables IR, Wi-Fi, pins or TCP. */
#include "ameba_soc.h" /* must come from the Realtek SDK, not a shipped shim */
#include "../core/gwir_protocol.h"
#include "../core/gwir_engine.h"
#include "../core/gwir_auth.h"
#include "gwir_sdk_hmac.h"
#include "../core/gwir_auth_dispatch.h"
#include "../realtek/gwir_amebad_tx_codec.h"
#include "../realtek/gwir_amebad_tx_fifo.h"
#include "../realtek/gwir_amebad_rx_fifo.h"
#include "../realtek/gwir_amebad_rx_irq.h"
#include "../realtek/gwir_amebad_bus.h"
#include "../realtek/gwir_amebad_vendor_bus.h"

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
    /* Check HMAC provider linking but reject a missing key before crypto. */
    ok = ok && (gwir_sdk_mbedtls_hmac(NULL, NULL, 32u, msg, sizeof(msg), unused_digest) == GWIR_SDK_HMAC_REJECT);
    /* Verify fail-closed physical-board gate, no IR register access. */
    ok = ok && native_bus_ops.board_ir_ready(native_bus_ops.ctx) == 0;
    DBG_8195A("[GWIR REV13] SDK link probe %s; e=%u,rxq=%u; IR/WiFi disabled\n",
              ok ? "OK" : "FAIL", (unsigned int)sizeof(gwir_engine),
              (unsigned int)sizeof(gwir_amebad_rx_queue));
}
