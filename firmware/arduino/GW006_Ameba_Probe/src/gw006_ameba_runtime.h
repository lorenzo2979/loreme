#ifndef GW006_AMEBA_RUNTIME_H
#define GW006_AMEBA_RUNTIME_H
#include <Arduino.h>
#include <WiFi.h>
#include "gw006_ameba_authenticated_tcp.h"
extern "C" {
#include "gwir_wbrg1_flash_port.h"
#include "gwir_amebad_vendor_bus.h"
#include "gwir_ir_pipeline.h"
}
/* Integration boundary for trusted, DEVICE-LOCAL provisioning.
 * No key, WiFi password, nonce, or flash partition permission is embedded
 * in the firmware. Whoever invokes begin() must first establish that:
 * - the red LED has been removed and PA25 is free for transistor IR TX;
 * - PA26 IR RX and its 3.3V receiver are correctly wired;
 * - the 8MiB board partition has been audited and assigned exclusively;
 * - key is device-unique; nonce is new and unpredictably generated PER BOOT;
 * - the Android owner knows the key+nonce via a trusted pairing channel.
 * Refuses activation otherwise. No OpenBeken interaction or flashing here.
 */
struct Gw006AmebaBootMaterial {
    char ssid[33];
    char wifi_password[65];
    uint8_t key[GWIR_AUTH_KEY_BYTES];
    uint8_t boot_nonce[GWIR_AUTH_NONCE_BYTES];
    int (*verified_board_hardware)(void *ctx);
    void *board_ctx;
};
class Gw006AmebaRuntime {
public:
    Gw006AmebaRuntime();
    bool begin(const Gw006AmebaBootMaterial &config);
    void poll();
    void stop();
    bool running() const { return running_; }
private:
    bool running_;
    bool pipeline_ready_;
    Gw006AmebaAuthenticatedTcp tcp_;
};
#endif
