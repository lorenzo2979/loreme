#include "gwir_auth_dispatch.h"
#include <openssl/hmac.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
static int hmac(void *ctx,const uint8_t *key,size_t kl,const uint8_t *data,
                size_t dl,uint8_t out[32]) {
    (void)ctx;
    unsigned int len=0;
    return !HMAC(EVP_sha256(),key,(int)kl,data,dl,out,&len)||len!=32;
}
static unsigned tx_counter;
static int tx(void *ctx,uint32_t hz,uint16_t duty,const uint32_t *dur,
              uint16_t n,uint32_t gap) {
    (void)ctx;(void)gap;
    assert(hz==38000 && duty==333 && n==2 && dur[0]==9000 && dur[1]==4500);
    ++tx_counter;return 0;
}
int main(void) {
    uint8_t key[32],nonce[16];
    for(unsigned i=0;i<32;++i) key[i]=(uint8_t)(i+1);
    for(unsigned i=0;i<16;++i) nonce[i]=(uint8_t)(0x40+i);
    gwir_auth_session phone_tx={0},device_rx={0},device_tx={0},phone_rx={0};
    assert(gwir_auth_init(&phone_tx,key,nonce,hmac,NULL)==0);
    assert(gwir_auth_init(&device_rx,key,nonce,hmac,NULL)==0);
    assert(gwir_auth_init(&device_tx,key,nonce,hmac,NULL)==0);
    assert(gwir_auth_init(&phone_rx,key,nonce,hmac,NULL)==0);
    static gwir_engine engine;
    gwir_ir_hal ops={0};ops.tx=tx;
    gwir_engine_init(&engine,ops);
    uint8_t payload[20]={0};
    gwir_write_u32be(payload,38000u);
    gwir_write_u16be(payload+4,333u);
    gwir_write_u16be(payload+6,2u);
    gwir_write_u32be(payload+8,0u);
    gwir_write_u32be(payload+12,9000u);
    gwir_write_u32be(payload+16,4500u);
    uint8_t inner[GWIR_MAX_MESSAGE];size_t innerlen=0;
    assert(gwir_encode(inner,sizeof(inner),GWIR_TX_FRAME,3,payload,20,&innerlen)==0);
    uint8_t request[GWIR_AUTH_MAX_PACKET], response[GWIR_AUTH_MAX_PACKET];
    uint8_t bad[GWIR_AUTH_MAX_PACKET];
    static uint8_t scratch[GWIR_MAX_MESSAGE];
    size_t reqsize=0,outsize=123;
    assert(gwir_auth_wrap(&phone_tx,GWIR_AUTH_TO_DEVICE,inner,innerlen,
                          request,sizeof(request),&reqsize)==0);
    memcpy(bad,request,reqsize);bad[reqsize-1]^=1;
    assert(gwir_auth_dispatch(&device_rx,&device_tx,&engine,bad,reqsize,
                              scratch,sizeof(scratch),response,sizeof(response),&outsize)<0);
    assert(outsize==0u && tx_counter==0u);
    assert(gwir_auth_dispatch(&device_rx,&device_tx,&engine,request,reqsize,
                              scratch,sizeof(scratch),response,sizeof(response),&outsize)==0);
    assert(tx_counter==1u && outsize>0u);
    const uint8_t *parsed=NULL;size_t len=0;
    assert(gwir_auth_unwrap(&phone_rx,GWIR_AUTH_FROM_DEVICE,response,outsize,&parsed,&len)==0);
    gwir_message reply={0};assert(gwir_decode(parsed,len,&reply)==0);
    assert(reply.command==GWIR_STATUS && reply.payload_len==1u && reply.payload[0]==GWIR_OK);
    assert(gwir_auth_dispatch(&device_rx,&device_tx,&engine,request,reqsize,
                              scratch,sizeof(scratch),response,sizeof(response),&outsize)==GWIR_AUTH_REPLAY);
    assert(tx_counter==1u && outsize==0u);
    assert(gwir_auth_wrap(&phone_tx,GWIR_AUTH_TO_DEVICE,inner,innerlen,
                          request,sizeof(request),&reqsize)==0);
    assert(gwir_auth_dispatch(&device_rx,&device_tx,&engine,request,reqsize,
                              scratch,3u,response,sizeof(response),&outsize)==GWIR_AUTH_REJECT);
    assert(tx_counter==1u && outsize==0u);
    assert(gwir_auth_dispatch(&device_rx,&device_tx,&engine,request,reqsize,
                              scratch,sizeof(scratch),response,sizeof(response),&outsize)==0);
    assert(tx_counter==2u);
    puts("PASS signed dispatch: unsigned/tampered/replayed TX cannot reach engine; bound buffers");
    return 0;
}
