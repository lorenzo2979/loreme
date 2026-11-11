#include "gwir_sdk_hmac.h"
#include "gwir_protocol.h"
#include "gwir_auth.h"
#include "mbedtls/md.h"
#include <assert.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <string.h>
extern int gwir_mock_md_fail;
static void test_vectors(void) {
    /* Public, non-secret KAT only: a 32-byte 0x0b key and ASCII 'Hi There'. */
    uint8_t key[GWIR_AUTH_KEY_BYTES], out[GWIR_AUTH_TAG_BYTES], ref[32];
    unsigned int n=0u;
    memset(key,0x0b,sizeof(key));
    assert(gwir_sdk_mbedtls_hmac(NULL,key,sizeof(key),
           (const uint8_t *)"Hi There",8u,out)==0);
    assert(HMAC(EVP_sha256(),key,(int)sizeof(key),
               (const uint8_t *)"Hi There",8u,ref,&n) && n==32u);
    assert(memcmp(ref,out,32u)==0);
    assert(gwir_sdk_mbedtls_hmac(NULL,key,20u,
           (const uint8_t *)"Hi There",8u,out)==GWIR_SDK_HMAC_REJECT);
    assert(gwir_sdk_mbedtls_hmac(NULL,key,sizeof(key),
           (const uint8_t *)"Hi There",0u,out)==GWIR_SDK_HMAC_REJECT);
    assert(gwir_sdk_mbedtls_hmac(NULL,NULL,sizeof(key),
           (const uint8_t *)"Hi There",8u,out)==GWIR_SDK_HMAC_REJECT);
    memset(out,0x55,sizeof(out));
    gwir_mock_md_fail=1;
    assert(gwir_sdk_mbedtls_hmac(NULL,key,sizeof(key),
           (const uint8_t *)"Hi There",8u,out)==GWIR_SDK_HMAC_CRYPTO_ERROR);
    for(size_t i=0;i<sizeof(out);++i) assert(out[i]==0u);
    gwir_mock_md_fail=0;
}
static void test_auth_roundtrip(void) {
    uint8_t key[32],nonce[16],payload[GWIR_HEADER_SIZE],packet[GWIR_AUTH_MAX_PACKET];
    for(unsigned i=0;i<32u;i++)key[i]=(uint8_t)(i+1u);
    for(unsigned i=0;i<16u;i++)nonce[i]=(uint8_t)(i+33u);
    size_t n=0u,wirelen=0u,opened_len=0u;
    assert(gwir_encode(payload,sizeof(payload),GWIR_PING,42u,NULL,0u,&n)==0);
    gwir_auth_session sender={0},receiver={0};
    assert(gwir_auth_init(&sender,key,nonce,gwir_sdk_mbedtls_hmac,NULL)==0);
    assert(gwir_auth_init(&receiver,key,nonce,gwir_sdk_mbedtls_hmac,NULL)==0);
    assert(gwir_auth_wrap(&sender,GWIR_AUTH_TO_DEVICE,payload,n,
                          packet,sizeof(packet),&wirelen)==0);
    const uint8_t *opened=NULL;
    assert(gwir_auth_unwrap(&receiver,GWIR_AUTH_TO_DEVICE,packet,wirelen,
                            &opened,&opened_len)==0);
    assert(opened_len==n && memcmp(payload,opened,n)==0);
    assert(gwir_auth_unwrap(&receiver,GWIR_AUTH_TO_DEVICE,packet,wirelen,
                            &opened,&opened_len)==GWIR_AUTH_REPLAY);
    gwir_auth_clear(&sender);gwir_auth_clear(&receiver);
}
int main(void) {
    test_vectors();test_auth_roundtrip();
    puts("PASS SDK-MbedTLS adapter: OpenSSL cross-check, missing crypto fail-closed, signed roundtrip, replay reject");
    return 0;
}
