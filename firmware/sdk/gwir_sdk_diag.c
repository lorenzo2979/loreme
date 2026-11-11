/* SDK linkage diagnostic only: never enables IR, Wi-Fi, pins or TCP. */
#include "ameba_soc.h" /* must come from the Realtek SDK, not a shipped shim */
#include "../core/gwir_protocol.h"
#include "../core/gwir_engine.h"
#include "../realtek/gwir_amebad_tx_codec.h"
#include "../realtek/gwir_amebad_tx_fifo.h"
#include "../realtek/gwir_amebad_rx_fifo.h"
#include "../realtek/gwir_amebad_rx_irq.h"

void gwir_sdk_integration_banner(void)
{
    /* Checks linker reachability and the vendor's real IR struct/API. */
    IR_InitTypeDef ir_cfg;
    uint8_t msg[GWIR_HEADER_SIZE];
    size_t msg_bytes = 0u;
    const uint32_t pulses[2] = {560u, 560u};
    uint32_t tx_words[2];
    gwir_amebad_tx_session s;
    int ok;

    /* IR_StructInit fills a local struct; it does not program the IR HW. */
    IR_StructInit(&ir_cfg);
    ok = (gwir_encode(msg, sizeof(msg), GWIR_PING, 1u, NULL, 0u,
                      &msg_bytes) == 0);
    ok = ok && msg_bytes == GWIR_HEADER_SIZE;
    ok = ok && (gwir_amebad_encode_tx(pulses, 2u, 38000u,
                                    tx_words, 2u) == 0);
    ok = ok && (gwir_amebad_tx_begin(&s, tx_words, 2u) == 0);
    /* No hardware register access: negative-argument path confirms linkage. */
    ok = ok && (gwir_amebad_rx_irq_service(NULL, 30000u, NULL) == -1);
    DBG_8195A("[GWIR REV10] SDK link probe %s; e=%u,rxq=%u; IR/WiFi disabled\n",
              ok ? "OK" : "FAIL", (unsigned int)sizeof(gwir_engine),
              (unsigned int)sizeof(gwir_amebad_rx_queue));
}
