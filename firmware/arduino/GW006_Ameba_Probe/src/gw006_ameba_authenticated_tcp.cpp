#include "gw006_ameba_authenticated_tcp.h"
#include <string.h>

namespace {
static constexpr uint32_t kIdleMs = 1200u;
static constexpr uint32_t kMaxClientMs = 10000u;
static constexpr uint16_t kGwAsPort = 42424u;

static int write_signed(void *arg, const uint8_t *bytes, size_t len) {
    WiFiClient *client = static_cast<WiFiClient *>(arg);
    if (!client || !bytes || !len || len > GWIR_AUTH_MAX_PACKET) return -1;
    size_t sent = 0u;
    while (sent < len) {
        if (!client->connected()) return -1;
        const size_t take = (len - sent) > 512u ? 512u : (len - sent);
        size_t n = client->write(bytes + sent, take);
        if (!n || n > take) return -1;
        sent += n;
    }
    return 0;
}
} // namespace

Gw006AmebaAuthenticatedTcp::Gw006AmebaAuthenticatedTcp()
  : server_(kGwAsPort), owner_(nullptr), pipeline_(nullptr), active_(false) {}

bool Gw006AmebaAuthenticatedTcp::begin(gwir_runtime_owner *owner,
                                        gwir_ir_pipeline *pipeline) {
    if (active_ || !owner || !owner->initialized || owner->connected ||
        !owner->rx.active || !owner->tx.active ||
        !pipeline || !pipeline->ready || pipeline->engine != owner->engine ||
        WiFi.status() != WL_CONNECTED) return false;
    owner_ = owner;
    pipeline_ = pipeline;
    server_.setNonBlockingMode();
    server_.begin();
    active_ = true;
    return true;
}

void Gw006AmebaAuthenticatedTcp::end() {
    if (!active_) return;
    server_.stop();
    if (owner_ && owner_->connected)
        gwir_runtime_owner_disconnect(owner_);
    owner_ = nullptr;
    pipeline_ = nullptr;
    active_ = false;
}

void Gw006AmebaAuthenticatedTcp::poll() {
    if (!active_) return;
    if (!owner_ || !owner_->initialized || !pipeline_ ||
        !pipeline_->ready || WiFi.status() != WL_CONNECTED) {
        end();
        return;
    }
    /* The IRQ handler queues raw samples; only the serialized owner loop
     * is permitted to turn these events into up to five IR captures. This
     * step must run even when no TCP client is currently connected. */
    if (pipeline_->receiving && pipeline_->engine->rx_armed &&
        gwir_ir_pipeline_pump(pipeline_, 128u) < 0) {
        end();
        return;
    }
    WiFiClient client = server_.available();
    if (!client) return;
    if (gwir_runtime_owner_connect(owner_) != 0) {
        client.stop();
        return;
    }

    uint8_t input[256];
    uint32_t begun = millis(), last = begun;
    while (client.connected() &&
           (uint32_t)(millis() - begun) < kMaxClientMs &&
           (uint32_t)(millis() - last) < kIdleMs) {
        int pending = client.available();
        if (pending <= 0) {
            delay(1);
            continue;
        }
        size_t maxRead = pending < (int)sizeof(input) ?
                         (size_t)pending : sizeof(input);
        int n = client.read(input, maxRead);
        if (n <= 0 || (size_t)n > maxRead) break;
        last = millis();
        int processed = gwir_runtime_owner_feed(owner_, input, (size_t)n,
                                                 write_signed, &client);
        if (processed < 0) break;
        /* At most one signed command for each serialized connection. This
         * also prevents a client holding an IR receiver indefinitely. */
        if (processed > 0) break;
    }
    memset(input, 0, sizeof(input));
    gwir_runtime_owner_disconnect(owner_);
    client.stop();
}
