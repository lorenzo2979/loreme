/*
 * GW006 IR Universal -- Arduino AmebaD toolchain compatibility probe.
 *
 * BUILD-ONLY: the official board selected by CI is AW-CU488 RTL8721DM,
 * NOT the GW006 WBRG1 RTL8721CSM. Compiling this sketch does NOT qualify
 * the image for flashing, boot, NOR writes, provisioning or physical IR.
 *
 * This intentionally tests the vendor C++ API at compile and linker stages.
 * No pin is configured; no Wi-Fi connection or flash operation is attempted
 * from setup()/loop().
 */
#include <Arduino.h>
#include <WiFi.h>
#include <IRDevice.h>
#include <FlashMemory.h>
#include "src/gw006_ameba_authenticated_tcp.h"
#include "src/gw006_ameba_hmac.h"
#include "src/gw006_ameba_runtime.h"
extern "C" {
#include "src/gwir_engine.h" // actual shared C engine compiled under src/
#include "src/gwir_runtime_owner.h" // signed GWAS, persistent 96-button NOR journal
#include "src/gwir_wbrg1_flash_port.h" // bounded 8MiB NOR adapter (inactive until approved)
#include "src/gwir_amebad_board_pins.h"
#include "src/gwir_amebad_vendor_bus.h"
#include "src/gwir_ir_pipeline.h"
}

static constexpr unsigned int kTVButtons = 36u;
static constexpr unsigned int kClimateButtons = 14u;
static constexpr unsigned int kDecoderButtons = 32u;
static constexpr unsigned int kSpeakerButtons = 14u;
static constexpr unsigned int kFramesPerButton = 5u;
static_assert(kTVButtons + kClimateButtons + kDecoderButtons + kSpeakerButtons == 96u,
              "GW006 exactly 96 buttons");
static_assert(kFramesPerButton == 5u, "GW006 exactly five IR frames per button");

/* Compile and LINK the underlying Wi-Fi, IR transmitter and FlashMemory
 * library methods. This function is NOT called at runtime, and is retained
 * only for static toolchain compatibility evidence. On actual GW006 use the
 * verified WBRG1 GPIO map rather than the AW-CU488 IR_TX pin.
 */
__attribute__((used, noinline))
void gw006_amebad_link_only_probe(void) {
    unsigned int sample[4] = {900u, 450u, 560u, 560u};
    uint8_t state = WiFi.status();
    IPAddress ip = WiFi.localIP();
    IR.begin((uint8_t)IR_TX, IR_MODE_TX, 38000u);
    IR.send(sample, 4u);
    unsigned int word = FlashMemory.readWord(0u);
    Serial.print((unsigned long)(state + ip[0] + word));
}

/* Real GWIR v3 parser and IR engine exercised on-device, with no IR HW
 * connected. Use a static object: the five-frame IR queue is too large
 * for the limited FreeRTOS task stack. */
/* The RTL8721D PSRAM linker section is provided by the official AmebaD
 * board package. Keep the ~50KB five-frame RX engine out of the limited
 * KM4 SRAM BSS; never allocate it on a FreeRTOS task stack. */
static gwir_engine s_gwir_engine __attribute__((section(".psram.bss")));
/* A trusted board-specific provisioner may supply Wi-Fi credentials, a
 * device-unique HMAC key, a fresh boot nonce and hardware approval.
 * There is deliberately NO fixed secret or default credential. */
extern "C" bool gw006_load_verified_boot_material(Gw006AmebaBootMaterial *)
    __attribute__((weak));
static Gw006AmebaRuntime s_runtime;
/* Real 21-module native IR HAL: check only metadata and fail-closed
 * hardware state. No GPIO pinmux or IRQ is activated by this test. */
static bool gw006_native_ir_contract_selftest(void) {
    gwir_board_pins pins = gwir_board_pins_candidate();
    gwir_amebad_bus_ops ops = gwir_amebad_vendor_bus_ops();
    return gwir_board_pins_validate(&pins) == 0 &&
           ops.board_ir_ready && ops.board_ir_ready(ops.ctx) == 0 &&
           gwir_ir_pipeline_init(nullptr, nullptr, nullptr) == -1;
}
static bool gw006_core_selftest(void) {
    uint8_t request[GWIR_HEADER_SIZE], response[GWIR_HEADER_SIZE];
    size_t request_len = 0u, response_len = 0u;
    gwir_message decoded;
    gwir_ir_hal disconnected = {};
    gwir_engine_init(&s_gwir_engine, disconnected);
    if (gwir_encode(request, sizeof(request), GWIR_PING, 1u,
                    NULL, 0u, &request_len) != 0 ||
        gwir_engine_process(&s_gwir_engine, request, request_len,
                            response, sizeof(response), &response_len) != 0 ||
        gwir_decode(response, response_len, &decoded) != 0)
        return false;
    return decoded.command == GWIR_PONG && decoded.sequence == 1u &&
           decoded.payload_len == 0u;
}
/* Exercise the actual authenticated journal / owner code and refuse
 * activation with absent keys, missing NOR region and no physical IR. */
static bool gw006_store_auth_contracts_selftest() {
    uint32_t bytes = gwir_fj_worst_case_rewrite_bytes();
    if (bytes < 4000000u || bytes > 8000000u) return false;
    if (gwir_fj_open(NULL, NULL) != -1) return false;
    if (gwir_wbrg1_flash_port_attach(NULL) != -1) return false;
    /* Flash attach(NULL) must reject prior to JEDEC read; NO I/O issued. */
    if (gwir_saved_play_init(NULL, NULL, NULL, NULL, 0u) != -1) return false;
    if (gwir_auth_init(NULL, NULL, NULL, NULL, NULL) != GWIR_AUTH_REJECT)
        return false;
    if (gwir_runtime_owner_init(NULL, NULL, NULL, NULL, NULL,
                                NULL, NULL, NULL, 0u, NULL, 0u) != -1)
        return false;
    return true;
}
/* A fixed PUBLIC self-test vector, NOT a device secret. Checks that
 * Realtek's HMAC implementation matches desktop SHA-256/HMAC exactly. */
static bool gw006_crypto_selftest() {
    const char text[] = "GWIR3-AmebaD-hmac-self-test";
    static const uint8_t expected[32] = {
        0x04,0xc4,0x3c,0x91,0x4f,0xb5,0x05,0x99,
        0xb0,0xe2,0xd0,0xd7,0xd9,0xe5,0x06,0xeb,
        0x7e,0xdf,0x85,0x15,0xd9,0x3d,0x95,0x03,
        0xc9,0x88,0xed,0x09,0xd0,0x82,0xc4,0x91
    };
    uint8_t key[32], digest[32];
    for (unsigned i=0;i<32u;i++) key[i]=(uint8_t)(i+1u);
    int rc=gw006_ameba_hmac_sha256(nullptr,key,sizeof(key),
                                   (const uint8_t*)text,sizeof(text)-1u,digest);
    bool ok=rc==0 && memcmp(digest,expected,sizeof(expected))==0;
    memset(key,0,sizeof(key));memset(digest,0,sizeof(digest));
    return ok;
}
void setup() {
    Serial.begin(115200);
    Serial.println("GW006 AMEBAD ARDUINO FAMILY BUILD - WBRG1 HARDWARE NOT YET SUPPORTED");
    Serial.println(gw006_core_selftest()
                   ? "GW006 REAL C PROTOCOL + IR ENGINE PING PASS"
                   : "GW006 REAL C PROTOCOL/IR ENGINE SELFTEST FAILED");
    Serial.println(gw006_native_ir_contract_selftest()
                   ? "GW006 NATIVE IR RX/TX PIPELINE LINKED; PINMUX REMAINS OFF"
                   : "GW006 NATIVE IR PIPELINE CONTRACT FAILED");
    Serial.println(gw006_store_auth_contracts_selftest()
                   ? "GW006 NINE CORE MODULES: AUTH + 96-KEY NOR JOURNAL CONTRACTS PASS"
                   : "GW006 AUTH/JOURNAL SOURCE CONTRACT TEST FAILED");
    Serial.println(gw006_crypto_selftest()
                   ? "GW006 AMEBAD REALTEK HMAC-SHA256 VECTOR PASS"
                   : "GW006 AMEBAD REALTEK HMAC-SHA256 VECTOR FAILED");
    Gw006AmebaBootMaterial config = {};
    if (gw006_load_verified_boot_material &&
        gw006_load_verified_boot_material(&config)) {
        if (!s_runtime.begin(config))
            Serial.println("GW006 BOARD RUNTIME STARTUP REFUSED");
    } else {
        Serial.println("GW006 UNPROVISIONED: IR/FLASH/WIFI NOT ACTIVATED");
    }
    memset(&config, 0, sizeof(config));
}
void loop() {
    s_runtime.poll();
    delay(2);
}
