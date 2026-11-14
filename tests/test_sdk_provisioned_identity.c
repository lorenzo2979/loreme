#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/sdk/gwir_sdk_provisioned_identity.h"

typedef struct {
    uint64_t epoch;
    unsigned approved, delivered, reserved, random_calls;
    int fail_key, fail_epoch, fail_rng, fail_delivery, erased_key, zero_rng;
} fake_board;
static int ok(void *ctx) { return ((fake_board *)ctx)->approved ? 1 : 0; }
static int key(void *ctx,uint8_t out[GWIR_AUTH_KEY_BYTES]) {
    fake_board *s=ctx;
    memset(out,s->erased_key ? 0xFF : 0x5A,GWIR_AUTH_KEY_BYTES);
    return s->fail_key ? -1 : 0;
}
static int reserve(void *ctx,uint64_t *out) {
    fake_board *s=ctx;
    ++s->reserved;
    if(s->fail_epoch) return -1;
    *out=++s->epoch;
    return 0;
}
static int rng(void *ctx,uint8_t out[8]) {
    fake_board *s=ctx;
    ++s->random_calls;
    memset(out,s->zero_rng ? 0 : 0xA7,8);
    return s->fail_rng ? -1 : 0;
}
static int deliver(void *ctx,const uint8_t n[GWIR_AUTH_NONCE_BYTES]) {
    fake_board *s=ctx;(void)n;
    ++s->delivered;
    return s->fail_delivery ? -1 : 0;
}
static void init(fake_board *b,gwir_sdk_identity_state *st,
                 gwir_sdk_secure_provider *p) {
    gwir_sdk_identity_io io={b,ok,key,reserve,rng,deliver};
    assert(gwir_sdk_identity_init(st,&io,p)==0);
}
int main(void) {
    fake_board b={.approved=1};
    gwir_sdk_identity_state state;
    gwir_sdk_secure_provider p;
    uint8_t k[GWIR_AUTH_KEY_BYTES],n[GWIR_AUTH_NONCE_BYTES],first[GWIR_AUTH_NONCE_BYTES];
    init(&b,&state,&p);
    assert(p.trust_verified(p.ctx)==1);
    assert(p.load_key(p.ctx,k)==0 && k[0]==0x5A);
    assert(p.fresh_nonce(p.ctx,n)==0);
    memcpy(first,n,sizeof(first));
    assert(p.nonce_freshness_verified(p.ctx,n)==1);
    assert(p.nonce_freshness_verified(p.ctx,n)==0);
    assert(p.deliver_nonce(p.ctx,n)==0 && b.delivered==1);
    assert(p.deliver_nonce(p.ctx,n)!=0 && b.delivered==1);
    gwir_sdk_identity_wipe(&state);
    init(&b,&state,&p);
    assert(p.load_key(p.ctx,k)==0);
    assert(p.fresh_nonce(p.ctx,n)==0);
    assert(b.epoch==2 && memcmp(first,n,sizeof(n))!=0);
    gwir_sdk_identity_wipe(&state);
    b.approved=0;
    init(&b,&state,&p);
    assert(p.load_key(p.ctx,k)!=0 && k[0]==0);
    b.approved=1;b.erased_key=1;
    assert(p.load_key(p.ctx,k)!=0 && k[0]==0);
    b.erased_key=0;b.fail_key=1;
    assert(p.load_key(p.ctx,k)!=0 && k[0]==0);
    b.fail_key=0;
    assert(p.load_key(p.ctx,k)==0);
    b.fail_epoch=1;
    assert(p.fresh_nonce(p.ctx,n)!=0);
    assert(p.fresh_nonce(p.ctx,n)!=0);
    assert(b.reserved==3 && b.random_calls==2);
    gwir_sdk_identity_wipe(&state);
    b.fail_epoch=0;b.fail_rng=1;
    init(&b,&state,&p);
    assert(p.load_key(p.ctx,k)==0);
    assert(p.fresh_nonce(p.ctx,n)!=0);
    assert(b.epoch==3);
    gwir_sdk_identity_wipe(&state);
    b.fail_rng=0;b.zero_rng=1;
    init(&b,&state,&p);
    assert(p.load_key(p.ctx,k)==0);
    assert(p.fresh_nonce(p.ctx,n)!=0);
    assert(b.epoch==4);
    gwir_sdk_identity_wipe(&state);
    b.zero_rng=0;b.fail_delivery=1;
    init(&b,&state,&p);
    assert(p.load_key(p.ctx,k)==0);
    assert(p.fresh_nonce(p.ctx,n)==0);
    n[0]^=1;
    assert(p.nonce_freshness_verified(p.ctx,n)==0);
    n[0]^=1;
    assert(p.nonce_freshness_verified(p.ctx,n)==1);
    assert(p.deliver_nonce(p.ctx,n)!=0);
    assert(p.deliver_nonce(p.ctx,n)!=0);
    assert(b.delivered==2);
    puts("PASS GW006 persisted monotonic nonce, secret redaction and failed-delivery replay guard");
    return 0;
}
