#include "gwir_web_rpc.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Transport fake only. HMAC/replay integrity is tested separately by the
 * existing native GWIR owner tests; no fake crypto is used for production. */
static unsigned connects, disconnects, feeds, emitted;
static int failure_mode;
int gwir_runtime_owner_connect(gwir_runtime_owner *owner) {
    if (!owner || owner->connected) return -1;
    owner->connected = 1;
    connects++;
    return 0;
}
int gwir_runtime_owner_feed(gwir_runtime_owner *owner,
    const uint8_t *request, size_t n,
    gwir_auth_stream_send_fn send, void *ctx) {
    uint8_t reply[GWIR_AUTH_HEADER_BYTES + GWIR_HEADER_SIZE + GWIR_AUTH_TAG_BYTES] = {0};
    (void)request; (void)n;
    if (!owner->connected) return -1;
    feeds++;
    if (failure_mode == 1) return -1;
    memcpy(reply, "GWAS", 4u);
    if (send(ctx, reply, sizeof(reply)) != 0) return -1;
    emitted++;
    if (failure_mode == 2)
        (void)send(ctx, reply, sizeof(reply));
    return failure_mode == 3 ? -1 : 1;
}
void gwir_runtime_owner_disconnect(gwir_runtime_owner *owner) {
    if (owner && owner->connected) {
        owner->connected = 0;
        disconnects++;
    }
}

int main(void) {
    gwir_runtime_owner owner = {0};
    uint8_t dummy_engine[1] = {0}, dummy_scratch[1] = {0}, dummy_signed[1] = {0};
    uint8_t request[GWIR_AUTH_HEADER_BYTES + GWIR_HEADER_SIZE + GWIR_AUTH_TAG_BYTES] = {0};
    uint8_t output[GWIR_AUTH_MAX_PACKET] = {0};
    size_t len = 123u;
    owner.initialized = 1u;
    owner.rx.active = owner.tx.active = 1u;
    owner.engine = (gwir_engine *)dummy_engine;
    owner.scratch = dummy_scratch;
    owner.signed_response = dummy_signed;
    memcpy(request, "GWAS", 4u);
    request[4] = GWIR_AUTH_VERSION;
    request[5] = GWIR_AUTH_TO_DEVICE;
    gwir_write_u32be(request+32u, GWIR_HEADER_SIZE);
    assert(gwir_web_rpc_process(&owner, request, sizeof(request), output,
                                sizeof(output), &len) == 0);
    assert(len == sizeof(request) && feeds == 1 && connects == 1 &&
           disconnects == 1 && !owner.connected);
    /* Reconnect preserves the per-boot counter; the bridge never re-inits. */
    owner.rx.last_rx_counter = 41u;
    assert(gwir_web_rpc_process(&owner, request, sizeof(request), output,
                                sizeof(output), &len) == 0);
    assert(owner.rx.last_rx_counter == 41u && connects == 2 && disconnects == 2);
    request[5] = GWIR_AUTH_FROM_DEVICE;
    assert(gwir_web_rpc_process(&owner, request, sizeof(request), output,
                                sizeof(output), &len) == -2 && len == 0u);
    request[5] = GWIR_AUTH_TO_DEVICE;
    assert(gwir_web_rpc_process(&owner, request, sizeof(request)-1u, output,
                                sizeof(output), &len) == -2);
    request[32] = 0xff;
    assert(gwir_web_rpc_process(&owner, request, sizeof(request), output,
                                sizeof(output), &len) == -2);
    request[32] = 0u;
    owner.rx.active = 0u;
    assert(gwir_web_rpc_process(&owner, request, sizeof(request), output,
                                sizeof(output), &len) == -1);
    owner.rx.active = 1u;
    failure_mode = 1;
    assert(gwir_web_rpc_process(&owner, request, sizeof(request), output,
                                sizeof(output), &len) == -4 && !owner.connected);
    failure_mode = 3;
    assert(gwir_web_rpc_process(&owner, request, sizeof(request), output,
                                sizeof(output), &len) == -4 && !owner.connected);
    assert(connects == disconnects);
    puts("GW006_WEB_RPC_TRANSPORT_GATE=PASS; HMAC verified by existing owner only; NO BOARD");
    return 0;
}
