#include "gwir_auth.h"
#include <assert.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int host_hmac(void *ctx, const uint8_t *key, size_t key_len,
                     const uint8_t *data, size_t data_len, uint8_t out[32]) {
    (void)ctx;
    unsigned int len=0;
    return key_len > 64u || !HMAC(EVP_sha256(),key,(int)key_len,data,data_len,out,&len)
           || len != 32u;
}
static int broken_hmac(void *ctx, const uint8_t *key, size_t kl,
                       const uint8_t *data, size_t dl, uint8_t out[32]) {
    (void)ctx;(void)key;(void)kl;(void)data;(void)dl;(void)out;
    return -1;
}
static void expect(int actual, int want) {
    if(actual != want) {fprintf(stderr,"got %d expected %d\n",actual,want);abort();}
}
static void tohex(const uint8_t *b,size_t n,char *out) {
    static const char HEX[]="0123456789abcdef";
    for(size_t i=0;i<n;++i){out[i*2]=HEX[b[i]>>4];out[i*2+1]=HEX[b[i]&15u];}
    out[n*2]='\0';
}
int main(void) {
    /* RFC 4231: HMAC-SHA256 test #1, vetted OpenSSL as host-only backend. */
    uint8_t tvkey[20];memset(tvkey,0x0b,sizeof(tvkey));
    uint8_t tvout[32];char hex[65];
    expect(host_hmac(NULL,tvkey,sizeof(tvkey),(const uint8_t *)"Hi There",8,tvout),0);
    tohex(tvout,32,hex);
    assert(strcmp(hex,"b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7")==0);

    uint8_t key[32], nonce[16];
    for(unsigned i=0;i<32;++i) key[i]=(uint8_t)(i+1u);
    for(unsigned i=0;i<16;++i) nonce[i]=(uint8_t)(0xa0u+i);
    uint8_t empty[32]={0};gwir_auth_session client={0},server={0};
    expect(gwir_auth_init(&client,empty,nonce,host_hmac,NULL),GWIR_AUTH_REJECT);
    expect(gwir_auth_init(&client,key,empty,host_hmac,NULL),GWIR_AUTH_REJECT);
    expect(gwir_auth_init(&client,key,nonce,NULL,NULL),GWIR_AUTH_REJECT);
    expect(gwir_auth_init(&client,key,nonce,host_hmac,NULL),0);
    expect(gwir_auth_init(&server,key,nonce,host_hmac,NULL),0);
    uint8_t inner[GWIR_MAX_MESSAGE];size_t in_size=0;
    expect(gwir_encode(inner,sizeof(inner),GWIR_PING,7u,(const uint8_t *)"hi",2u,&in_size),0);
    uint8_t wire[GWIR_AUTH_MAX_PACKET];size_t sz=0;
    expect(gwir_auth_wrap(&client,GWIR_AUTH_TO_DEVICE,inner,in_size,wire,sizeof(wire),&sz),0);
    assert(sz==GWIR_AUTH_HEADER_BYTES+in_size+GWIR_AUTH_TAG_BYTES && client.tx_counter==1u);
    assert(memcmp(wire,"GWAS",4)==0 && wire[4]==1 && wire[5]==1);
    /* Cross-language fixture: compare C with JCA and Python HMAC. */
    char full_hex[sizeof(wire)*2u+1u];tohex(wire,sz,full_hex);
    puts(full_hex);
    const uint8_t *decoded=NULL;size_t dec_size=0;
    expect(gwir_auth_unwrap(&server,GWIR_AUTH_FROM_DEVICE,wire,sz,&decoded,&dec_size),GWIR_AUTH_REJECT);
    expect(gwir_auth_unwrap(&server,GWIR_AUTH_TO_DEVICE,wire,sz,&decoded,&dec_size),0);
    assert(dec_size==in_size && memcmp(decoded,inner,in_size)==0 && server.last_rx_counter==1u);
    expect(gwir_auth_unwrap(&server,GWIR_AUTH_TO_DEVICE,wire,sz,&decoded,&dec_size),GWIR_AUTH_REPLAY);
    assert(decoded==NULL && dec_size==0);

    /* Even a single changed byte, length or nonce fails HMAC or framing. */
    uint8_t changed[GWIR_AUTH_MAX_PACKET];
    memcpy(changed,wire,sz);changed[GWIR_AUTH_HEADER_BYTES+20u]^=1u;
    gwir_auth_session other={0};expect(gwir_auth_init(&other,key,nonce,host_hmac,NULL),0);
    expect(gwir_auth_unwrap(&other,GWIR_AUTH_TO_DEVICE,changed,sz,&decoded,&dec_size),GWIR_AUTH_REJECT);
    assert(other.last_rx_counter==0u);
    memcpy(changed,wire,sz);changed[32u]^=1u;
    expect(gwir_auth_unwrap(&other,GWIR_AUTH_TO_DEVICE,changed,sz,&decoded,&dec_size),GWIR_AUTH_REJECT);
    memcpy(changed,wire,sz);changed[8u]^=1u;
    expect(gwir_auth_unwrap(&other,GWIR_AUTH_TO_DEVICE,changed,sz,&decoded,&dec_size),GWIR_AUTH_REJECT);
    expect(gwir_auth_unwrap(&other,GWIR_AUTH_TO_DEVICE,wire,sz-1u,&decoded,&dec_size),GWIR_AUTH_REJECT);
    expect(gwir_auth_unwrap(&other,GWIR_AUTH_TO_DEVICE,wire,sz,&decoded,&dec_size),0);

    /* Old data cannot be replayed even with a perfect CRC. */
    expect(gwir_auth_wrap(&client,GWIR_AUTH_TO_DEVICE,inner,in_size,wire,sizeof(wire),&sz),0);
    expect(gwir_auth_unwrap(&server,GWIR_AUTH_TO_DEVICE,wire,sz,&decoded,&dec_size),0);
    assert(server.last_rx_counter==2u);
    expect(gwir_auth_unwrap(&server,GWIR_AUTH_TO_DEVICE,wire,sz,&decoded,&dec_size),GWIR_AUTH_REPLAY);
    uint8_t fresh_nonce[16];memcpy(fresh_nonce,nonce,16);fresh_nonce[1]^=0x55;
    gwir_auth_session after_reboot={0};expect(gwir_auth_init(&after_reboot,key,fresh_nonce,host_hmac,NULL),0);
    expect(gwir_auth_unwrap(&after_reboot,GWIR_AUTH_TO_DEVICE,wire,sz,&decoded,&dec_size),GWIR_AUTH_REJECT);

    /* Deny any command when cryptographic engine fails. */
    gwir_auth_session unavailable={0};
    expect(gwir_auth_init(&unavailable,key,nonce,broken_hmac,NULL),0);
    expect(gwir_auth_unwrap(&unavailable,GWIR_AUTH_TO_DEVICE,wire,sz,&decoded,&dec_size),GWIR_AUTH_CRYPTO_ERROR);
    expect(gwir_auth_wrap(&unavailable,GWIR_AUTH_TO_DEVICE,inner,in_size,changed,sizeof(changed),&dec_size),GWIR_AUTH_CRYPTO_ERROR);
    assert(dec_size==0u && unavailable.tx_counter==0u && changed[0]==0u);

    /* In-place or partially overlapping encode must be rejected BEFORE writes. */
    uint8_t overlapping[GWIR_AUTH_MAX_PACKET]={0};
    memcpy(overlapping+64u,inner,in_size);
    expect(gwir_auth_wrap(&client,GWIR_AUTH_TO_DEVICE,overlapping+64u,in_size,
                          overlapping,sizeof(overlapping),&dec_size),GWIR_AUTH_REJECT);
    assert(dec_size==0u && memcmp(overlapping+64u,inner,in_size)==0);

    /* Full maximum-sized GWIR v3 payload fits in the secure envelope. */
    uint8_t maximum[GWIR_MAX_MESSAGE];
    uint8_t maxpayload[GWIR_MAX_PAYLOAD]={0};
    size_t max_inner_len=0,max_signed_len=0;
    expect(gwir_encode(maximum,sizeof(maximum),GWIR_PING,19u,
                       maxpayload,sizeof(maxpayload),&max_inner_len),0);
    assert(max_inner_len==GWIR_MAX_MESSAGE);
    expect(gwir_auth_wrap(&client,GWIR_AUTH_TO_DEVICE,maximum,max_inner_len,
                          wire,sizeof(wire),&max_signed_len),0);
    assert(max_signed_len==GWIR_AUTH_MAX_PACKET);
    expect(gwir_auth_unwrap(&server,GWIR_AUTH_TO_DEVICE,wire,max_signed_len,
                            &decoded,&dec_size),0);
    assert(dec_size==GWIR_MAX_MESSAGE);

    /* High sequence and invalid bounds: no signed wrap. */
    client.tx_counter=GWIR_AUTH_MAX_COUNTER;
    expect(gwir_auth_wrap(&client,GWIR_AUTH_TO_DEVICE,inner,in_size,changed,sizeof(changed),&dec_size),GWIR_AUTH_REJECT);
    expect(gwir_auth_wrap(&client,GWIR_AUTH_TO_DEVICE,inner,in_size,changed,10u,&dec_size),GWIR_AUTH_REJECT);
    gwir_auth_clear(&client);gwir_auth_clear(&server);gwir_auth_clear(&other);
    gwir_auth_clear(&unavailable);gwir_auth_clear(&after_reboot);
    for(size_t i=0;i<sizeof(client);++i) assert(((uint8_t *)&client)[i]==0u);
    puts("PASS: authenticated GWAS envelope HMAC-SHA256, anti-replay, nonce rotation, fail-closed");
    return 0;
}
