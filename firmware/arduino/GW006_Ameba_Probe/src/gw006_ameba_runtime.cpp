#include "gw006_ameba_runtime.h"
#include "gw006_ameba_hmac.h"
#include <string.h>
extern "C" {
#include "ameba_soc.h"
#include "gwir_saved_play_command.h"
int gwir_amebad_vendor_board_ir_setup(void);
}

namespace {
/* WBRG1 includes 4MiB PSRAM. These large objects MUST NOT be on an
 * Arduino/FreeRTOS task stack, nor in its 456KiB limited KM4 internal RAM.
 * One owner serializes ALL operations on flash, IR and authentication. */
gwir_engine engine __attribute__((section(".psram.bss")));
gwir_ir_pipeline pipeline __attribute__((section(".psram.bss")));
uint8_t upload[GWIR_GWCD_MAX_SIZE] __attribute__((section(".psram.bss")));
uint32_t pulse[GWIR_MAX_PULSES_PER_FRAME] __attribute__((section(".psram.bss")));
uint8_t scratch[GWIR_MAX_MESSAGE] __attribute__((section(".psram.bss")));
uint8_t response[GWIR_AUTH_MAX_PACKET] __attribute__((section(".psram.bss")));
gwir_runtime_owner owner;
gwir_fj_store store;
gwir_fj_io nor;
gwir_saved_play_service saved;
gwir_fj_play_ops send_ops;

void lock_ir(void *)   { InterruptDis(IR_IRQ); }
void unlock_ir(void *) { InterruptEn(IR_IRQ, 10); }
int yield_ir(void *)   { delay(1); return 0; }
int ir_ready(void *ctx) {
    gwir_ir_pipeline *p = static_cast<gwir_ir_pipeline *>(ctx);
    return p && p->ready && p->bus.ops.board_ir_ready &&
           p->bus.ops.board_ir_ready(p->bus.ops.ctx) == 1 ? 1 : 0;
}
int send_saved_frame(void *ctx, uint32_t carrier, uint16_t duty,
                     const uint32_t *durations, uint16_t count, uint32_t gap) {
    gwir_ir_pipeline *p = static_cast<gwir_ir_pipeline *>(ctx);
    if (ir_ready(ctx) != 1 || !p->engine || !p->engine->hal.tx) return -1;
    return p->engine->hal.tx(p->engine->hal.ctx, carrier, duty, durations, count, gap);
}
bool nonempty(const uint8_t *p, size_t n) {
    uint8_t x = 0u, y = 0u;
    for (size_t i = 0; i < n; ++i) { x |= p[i]; y |= (uint8_t)~p[i]; }
    return x != 0u && y != 0u;
}
} // namespace

Gw006AmebaRuntime::Gw006AmebaRuntime()
    : running_(false), pipeline_ready_(false) {}

bool Gw006AmebaRuntime::begin(const Gw006AmebaBootMaterial &config) {
    if (running_ || pipeline_ready_ || owner.initialized ||
        !config.verified_board_hardware ||
        config.verified_board_hardware(config.board_ctx) != 1 ||
        config.ssid[0] == 0 || config.ssid[32] != 0 ||
        config.wifi_password[64] != 0 ||
        !nonempty(config.key, GWIR_AUTH_KEY_BYTES) ||
        !nonempty(config.boot_nonce, GWIR_AUTH_NONCE_BYTES))
        return false;
    /* No erase or write unless the reviewed Realtek NOR port's explicit
     * compile-time partition approval and JEDEC density gate both pass.
     * An empty GW006 with OpenBeken is NOT an automatically free partition. */
    if (gwir_wbrg1_flash_port_attach(&nor) != 0 ||
        gwir_fj_open(&store, &nor) != 0)
        return false;
    /* The physical PA25/PA26 pinmux is never enabled by a compile-only test.
     * A board-reviewed build must deliberately enable the vendor driver. */
    if (gwir_amebad_vendor_board_ir_setup() != 0)
        return false;

    gwir_ir_pipeline_config ic = {};
    ic.bus = gwir_amebad_vendor_bus_ops();
    ic.irq_lock.lock_ir_irq = lock_ir;
    ic.irq_lock.unlock_ir_irq = unlock_ir;
    ic.sample_hz = 10000000u;
    ic.carrier_guess_hz = 38000u;
    ic.duty_guess_permille = 333u;
    ic.active_mark_level = 0u;
    ic.gap_threshold_us = 30000u;
    ic.yield_one_ms = yield_ir;
    if (gwir_ir_pipeline_init(&pipeline, &engine, &ic) != 0)
        return false;
    pipeline_ready_ = true;

    send_ops.ctx = &pipeline;
    send_ops.board_ir_ready = ir_ready;
    send_ops.send_frame = send_saved_frame;
    if (gwir_saved_play_init(&saved, &store, &send_ops, pulse,
                             GWIR_MAX_PULSES_PER_FRAME) != 0 ||
        gwir_saved_play_enable_upload(&saved, upload, sizeof(upload)) != 0) {
        stop();
        return false;
    }
    /* GWAS uses a fresh per-boot nonce and HMAC-SHA256 (never CRC auth).
     * The caller is required to deliver the nonce over a trusted channel. */
    if (gwir_runtime_owner_init(&owner, &engine, &saved,
             config.key, config.boot_nonce, gw006_ameba_hmac_sha256, nullptr,
             scratch, sizeof(scratch), response, sizeof(response)) != 0) {
        stop();
        return false;
    }

    char ssid[33], password[65];
    memcpy(ssid, config.ssid, sizeof(ssid));
    memcpy(password, config.wifi_password, sizeof(password));
    if (WiFi.status() != WL_CONNECTED) {
        int rc = WiFi.begin(ssid, password);
        uint32_t start = millis();
        while (rc != WL_CONNECTED && WiFi.status() != WL_CONNECTED &&
               (uint32_t)(millis() - start) < 15000u)
            delay(50);
    }
    memset(password, 0, sizeof(password));
    memset(ssid, 0, sizeof(ssid));
    if (WiFi.status() != WL_CONNECTED || !tcp_.begin(&owner, &pipeline)) {
        stop();
        return false;
    }
    running_ = true;
    return true;
}

void Gw006AmebaRuntime::poll() {
    if (running_) {
        tcp_.poll();
        if (WiFi.status() != WL_CONNECTED || !pipeline.ready) stop();
    }
}

void Gw006AmebaRuntime::stop() {
    tcp_.end();
    if (owner.initialized) gwir_runtime_owner_clear(&owner);
    if (pipeline_ready_) {
        (void)gwir_ir_pipeline_stop(&pipeline);
        pipeline_ready_ = false;
    }
    memset(upload, 0, sizeof(upload));
    running_ = false;
}
