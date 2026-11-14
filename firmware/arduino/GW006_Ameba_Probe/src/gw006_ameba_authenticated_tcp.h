#ifndef GW006_AMEBA_AUTHENTICATED_TCP_H
#define GW006_AMEBA_AUTHENTICATED_TCP_H

#include <Arduino.h>
#include <WiFi.h>
extern "C" {
#include "gwir_runtime_owner.h"
#include "gwir_ir_pipeline.h"
}

/* AmebaD Arduino TCP adaptor around the ACTUAL GWAS HMAC authenticated
 * IR/storage owner. No unsigned fallback, no baked-in Wi-Fi credentials.
 * Only one socket at a time; no reset of replay counters on reconnect.
 * Caller must provision a 32-byte key, fresh boot nonce, HMAC provider,
 * IR HAL, exclusive NOR region and STA Wi-Fi BEFORE begin().
 */
class Gw006AmebaAuthenticatedTcp {
public:
    Gw006AmebaAuthenticatedTcp();
    bool begin(gwir_runtime_owner *owner, gwir_ir_pipeline *pipeline);
    void poll();
    void end();
private:
    WiFiServer server_;
    gwir_runtime_owner *owner_;
    gwir_ir_pipeline *pipeline_;
    bool active_;
};
#endif
