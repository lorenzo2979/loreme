#include "gwir_sdk_provisioned_identity.h"
#include <string.h>

static void wipe(void *value, size_t count)
{
    volatile uint8_t *p = (volatile uint8_t *)value;
    while (count--) *p++ = 0u;
}

static int approved(void *ctx)
{
    gwir_sdk_identity_state *s = (gwir_sdk_identity_state *)ctx;
    return (s && s->stage == 0u && s->io.approved &&
            s->io.approved(s->io.ctx) == 1) ? 1 : 0;
}

static int load_key(void *ctx, uint8_t key[GWIR_AUTH_KEY_BYTES])
{
    gwir_sdk_identity_state *s = (gwir_sdk_identity_state *)ctx;
    uint8_t nonzero = 0u, nonff = 0u;
    if (!key) return -1;
    memset(key, 0, GWIR_AUTH_KEY_BYTES);
    if (approved(ctx) != 1 || !s->io.read_device_key ||
        s->io.read_device_key(s->io.ctx, key) != 0) {
        wipe(key, GWIR_AUTH_KEY_BYTES);
        return -1;
    }
    for (unsigned i = 0; i < GWIR_AUTH_KEY_BYTES; ++i) {
        nonzero |= key[i];
        nonff |= (uint8_t)~key[i];
    }
    if (!nonzero || !nonff) {
        wipe(key, GWIR_AUTH_KEY_BYTES);
        return -1;
    }
    s->stage = 1u;
    return 0;
}

static int fresh_nonce(void *ctx, uint8_t nonce[GWIR_AUTH_NONCE_BYTES])
{
    gwir_sdk_identity_state *s = (gwir_sdk_identity_state *)ctx;
    uint64_t epoch = 0u;
    uint8_t random8[8] = {0};
    uint8_t nonzero = 0u;
    if (!nonce) return -1;
    memset(nonce, 0, GWIR_AUTH_NONCE_BYTES);
    if (!s || s->stage != 1u || !s->io.reserve_epoch || !s->io.random8)
        return -1;
    s->stage = 5u; /* irreversibly consume this session attempt */
    if (s->io.reserve_epoch(s->io.ctx, &epoch) != 0 || epoch == 0u)
        return -1;
    if (s->io.random8(s->io.ctx, random8) != 0) {
        wipe(random8, sizeof(random8));
        return -1;
    }
    for (unsigned i = 0; i < 8; ++i) nonzero |= random8[i];
    if (!nonzero) {
        wipe(random8, sizeof(random8));
        return -1;
    }
    for (unsigned i = 0; i < 8; ++i)
        nonce[i] = (uint8_t)(epoch >> (56u - 8u * i));
    memcpy(nonce + 8u, random8, sizeof(random8));
    memcpy(s->expected_nonce, nonce, GWIR_AUTH_NONCE_BYTES);
    wipe(random8, sizeof(random8));
    s->stage = 2u;
    return 0;
}

static int freshness_verified(void *ctx,
                              const uint8_t nonce[GWIR_AUTH_NONCE_BYTES])
{
    gwir_sdk_identity_state *s = (gwir_sdk_identity_state *)ctx;
    uint8_t difference = 0u;
    if (!s || s->stage != 2u || !nonce) return 0;
    for (unsigned i = 0; i < GWIR_AUTH_NONCE_BYTES; ++i)
        difference |= nonce[i] ^ s->expected_nonce[i];
    if (difference) return 0;
    s->stage = 3u;
    return 1;
}

static int deliver_nonce(void *ctx,
                         const uint8_t nonce[GWIR_AUTH_NONCE_BYTES])
{
    gwir_sdk_identity_state *s = (gwir_sdk_identity_state *)ctx;
    uint8_t difference = 0u;
    if (!s || s->stage != 3u || !nonce || !s->io.deliver_secure)
        return -1;
    for (unsigned i = 0; i < GWIR_AUTH_NONCE_BYTES; ++i)
        difference |= nonce[i] ^ s->expected_nonce[i];
    if (difference) return -1;
    s->stage = 4u; /* no repeat, even if peer rejects the message */
    int rc = s->io.deliver_secure(s->io.ctx, nonce);
    wipe(s->expected_nonce, sizeof(s->expected_nonce));
    return rc == 0 ? 0 : -1;
}

int gwir_sdk_identity_init(gwir_sdk_identity_state *state,
                           const gwir_sdk_identity_io *io,
                           gwir_sdk_secure_provider *provider)
{
    if (!state || !provider || !io || !io->approved ||
        !io->read_device_key || !io->reserve_epoch ||
        !io->random8 || !io->deliver_secure) return -1;
    memset(state, 0, sizeof(*state));
    memset(provider, 0, sizeof(*provider));
    state->io = *io;
    provider->ctx = state;
    provider->trust_verified = approved;
    provider->load_key = load_key;
    provider->fresh_nonce = fresh_nonce;
    provider->nonce_freshness_verified = freshness_verified;
    provider->deliver_nonce = deliver_nonce;
    return 0;
}
void gwir_sdk_identity_wipe(gwir_sdk_identity_state *state)
{
    if (state) wipe(state, sizeof(*state));
}
