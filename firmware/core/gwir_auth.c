#include "gwir_auth.h"
#include <string.h>

static int nonzero(const uint8_t *p, size_t n) {
    uint8_t any = 0u;
    for (size_t i = 0; i < n; ++i) any |= p[i];
    return any != 0u;
}
static void secure_wipe(void *v, size_t n) {
    volatile uint8_t *p = (volatile uint8_t *)v;
    while (n--) *p++ = 0;
}
static void put64(uint8_t *p, uint64_t x) {
    for (unsigned i=0u;i<8u;++i) p[i] = (uint8_t)(x >> (56u-i*8u));
}
static uint64_t get64(const uint8_t *p) {
    uint64_t x=0u;
    for (unsigned i=0u;i<8u;++i) x = (x<<8u)|p[i];
    return x;
}
static int valid(const gwir_auth_session *s) {
    return s && s->active == 1u && s->hmac != NULL;
}
static int direction_ok(uint8_t x) {
    return x == GWIR_AUTH_TO_DEVICE || x == GWIR_AUTH_FROM_DEVICE;
}
/* Reject overlapping input/output for a framed record before writing.
 * Comparing integer address differences avoids UB of cross-object pointers. */
static int ranges_overlap(const void *a, size_t na, const void *b, size_t nb) {
    uintptr_t aa=(uintptr_t)a, bb=(uintptr_t)b;
    return aa <= bb ? bb-aa < na : aa-bb < nb;
}
static int eq32(const uint8_t *a, const uint8_t *b) {
    uint8_t diff=0u;
    for (size_t i=0u;i<GWIR_AUTH_TAG_BYTES;++i) diff|=(uint8_t)(a[i]^b[i]);
    return diff == 0u;
}
void gwir_auth_clear(gwir_auth_session *s) {
    if (!s) return;
    secure_wipe(s, sizeof(*s));
}
int gwir_auth_init(gwir_auth_session *s, const uint8_t key[GWIR_AUTH_KEY_BYTES],
                   const uint8_t boot_nonce[GWIR_AUTH_NONCE_BYTES],
                   gwir_auth_hmac_fn hmac, void *crypto_ctx) {
    if (!s || !key || !boot_nonce || !hmac ||
        !nonzero(key, GWIR_AUTH_KEY_BYTES) ||
        !nonzero(boot_nonce, GWIR_AUTH_NONCE_BYTES)) return GWIR_AUTH_REJECT;
    gwir_auth_clear(s);
    memcpy(s->key,key,GWIR_AUTH_KEY_BYTES);
    memcpy(s->boot_nonce,boot_nonce,GWIR_AUTH_NONCE_BYTES);
    s->hmac = hmac;
    s->crypto_ctx = crypto_ctx;
    s->active = 1u;
    return GWIR_AUTH_OK;
}
int gwir_auth_wrap(gwir_auth_session *s, uint8_t direction,
                   const uint8_t *inner, size_t inner_len,
                   uint8_t *out, size_t cap, size_t *out_len) {
    if (out_len) *out_len=0u;
    if (!valid(s) || !direction_ok(direction) || !inner || !out || !out_len ||
        inner_len < GWIR_HEADER_SIZE || inner_len > GWIR_MAX_MESSAGE ||
        cap < GWIR_AUTH_HEADER_BYTES + inner_len + GWIR_AUTH_TAG_BYTES ||
        s->tx_counter >= GWIR_AUTH_MAX_COUNTER ||
        ranges_overlap(inner,inner_len,out,
                       GWIR_AUTH_HEADER_BYTES+inner_len+GWIR_AUTH_TAG_BYTES))
        return GWIR_AUTH_REJECT;
    gwir_message parsed;
    if (gwir_decode(inner, inner_len, &parsed)) return GWIR_AUTH_REJECT;
    uint64_t next=s->tx_counter+1u;
    memcpy(out,"GWAS",4u);
    out[4]=GWIR_AUTH_VERSION;
    out[5]=direction;
    out[6]=0u;out[7]=0u;
    memcpy(out+8u,s->boot_nonce,GWIR_AUTH_NONCE_BYTES);
    put64(out+24u,next);
    gwir_write_u32be(out+32u,(uint32_t)inner_len);
    memcpy(out+GWIR_AUTH_HEADER_BYTES,inner,inner_len);
    size_t authenticated=GWIR_AUTH_HEADER_BYTES+inner_len;
    if (s->hmac(s->crypto_ctx,s->key,sizeof(s->key),out,authenticated,
                out+authenticated)) {
        secure_wipe(out,authenticated+GWIR_AUTH_TAG_BYTES);
        return GWIR_AUTH_CRYPTO_ERROR;
    }
    s->tx_counter=next;
    *out_len=authenticated+GWIR_AUTH_TAG_BYTES;
    return GWIR_AUTH_OK;
}
int gwir_auth_unwrap(gwir_auth_session *s, uint8_t expected_direction,
                     const uint8_t *record, size_t size,
                     const uint8_t **inner, size_t *inner_len) {
    if (inner) *inner=NULL;
    if (inner_len) *inner_len=0u;
    if (!valid(s) || !direction_ok(expected_direction) || !record || !inner ||
        !inner_len || size < GWIR_AUTH_HEADER_BYTES+GWIR_HEADER_SIZE+GWIR_AUTH_TAG_BYTES ||
        size > GWIR_AUTH_MAX_PACKET) return GWIR_AUTH_REJECT;
    if (memcmp(record,"GWAS",4u) != 0 || record[4]!=GWIR_AUTH_VERSION ||
        record[5]!=expected_direction || record[6]!=0u || record[7]!=0u ||
        memcmp(record+8u,s->boot_nonce,GWIR_AUTH_NONCE_BYTES)!=0) return GWIR_AUTH_REJECT;
    uint64_t counter=get64(record+24u);
    if (counter == 0u || counter > GWIR_AUTH_MAX_COUNTER) return GWIR_AUTH_REJECT;
    if (counter <= s->last_rx_counter) return GWIR_AUTH_REPLAY;
    uint32_t size_inner=gwir_read_u32be(record+32u);
    if (size_inner<GWIR_HEADER_SIZE || size_inner>GWIR_MAX_MESSAGE ||
        size != GWIR_AUTH_HEADER_BYTES+(size_t)size_inner+GWIR_AUTH_TAG_BYTES)
        return GWIR_AUTH_REJECT;
    size_t authenticated=GWIR_AUTH_HEADER_BYTES+(size_t)size_inner;
    uint8_t computed[GWIR_AUTH_TAG_BYTES];
    if (s->hmac(s->crypto_ctx,s->key,sizeof(s->key),record,authenticated,computed)) {
        secure_wipe(computed,sizeof(computed));
        return GWIR_AUTH_CRYPTO_ERROR;
    }
    int match=eq32(computed,record+authenticated);
    secure_wipe(computed,sizeof(computed));
    if (!match) return GWIR_AUTH_REJECT;
    gwir_message parsed;
    if (gwir_decode(record+GWIR_AUTH_HEADER_BYTES,size_inner,&parsed))
        return GWIR_AUTH_REJECT;
    s->last_rx_counter=counter;
    *inner=record+GWIR_AUTH_HEADER_BYTES;
    *inner_len=size_inner;
    return GWIR_AUTH_OK;
}
