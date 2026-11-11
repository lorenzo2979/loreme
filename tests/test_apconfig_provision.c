#include "gwir_apconfig_provision.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static int stages, verifications, commits, rollbacks, fail_stage, fail_verify, fail_commit;
static int stage(void *c, const gwir_provision_profile *p) {
    (void)c; assert(strcmp(p->ssid,"My Router")==0);++stages;return fail_stage?-1:0;
}
static int verify(void *c) {(void)c;++verifications;return fail_verify?-1:0;}
static int commit(void *c) {(void)c;++commits;return fail_commit?-1:0;}
static void rollback(void *c) {(void)c;++rollbacks;}
static void reset(void) {stages=verifications=commits=rollbacks=0;fail_stage=fail_verify=fail_commit=0;}
static void test_parse(void) {
    gwir_provision_profile p;
    const char *s="ssid=My+Router&password=12345678";
    assert(gwir_provision_parse_form((const uint8_t *)s,strlen(s),&p)==0);
    assert(strcmp(p.ssid,"My Router")==0 && strcmp(p.password,"12345678")==0 && !p.is_open);
    s="password=&ssid=Cafe%20Wifi";
    assert(gwir_provision_parse_form((const uint8_t *)s,strlen(s),&p)==0);
    assert(strcmp(p.ssid,"Cafe Wifi")==0 && p.is_open);
    s="ssid=X&password=x%2Btest%21ok";
    assert(gwir_provision_parse_form((const uint8_t *)s,strlen(s),&p)==0);
    assert(strcmp(p.password,"x+test!ok")==0);
    const char *bad[]={"ssid=X","password=12345678","ssid=X&password=a",
        "ssid=X&password=12345678&ssid=Y","ssid=X&password=12345678&foo=1",
        "ssid=X&password=12345678&","ssid=X&password=%00AAAAAAA",
        "ssid=X&password=%Q012345678","ssid=X&password=abc%",
        "ssid=%0a&password=12345678","ssid=X&password=aaa%7faaaa",
        "ssid=&password=12345678","ssid=X&password=12345678&password=12345678"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i){
        memset(&p,0x41,sizeof(p));
        assert(gwir_provision_parse_form((const uint8_t *)bad[i],strlen(bad[i]),&p)!=0);
        const uint8_t *ptr=(const uint8_t *)&p;
        for(size_t j=0;j<sizeof(p);++j) assert(ptr[j]==0u);
    }
    uint8_t over[GWIR_PROVISION_BODY_MAX+1u];memset(over,'A',sizeof(over));
    assert(gwir_provision_parse_form(over,sizeof(over),&p)!=0);
    assert(gwir_provision_parse_form(NULL,4,&p)!=0);
    assert(gwir_provision_parse_form(over,4,NULL)!=0);
}
static void test_apply(void) {
    gwir_provision_profile p;
    gwir_provision_ops ops={0,stage,verify,commit,rollback};
    const char *s="ssid=My+Router&password=12345678";
    assert(gwir_provision_parse_form((const uint8_t *)s,strlen(s),&p)==0);
    reset();assert(gwir_provision_apply(&p,&ops)==0);
    assert(stages==1 && verifications==1 && commits==1 && rollbacks==0);
    reset();fail_stage=1;assert(gwir_provision_apply(&p,&ops)==-3);
    assert(stages==1 && verifications==0 && commits==0 && rollbacks==1);
    reset();fail_verify=1;assert(gwir_provision_apply(&p,&ops)==-4);
    assert(stages==1 && verifications==1 && commits==0 && rollbacks==1);
    reset();fail_commit=1;assert(gwir_provision_apply(&p,&ops)==-5);
    assert(stages==1 && verifications==1 && commits==1 && rollbacks==1);
    assert(gwir_provision_apply(&p,NULL)==-1);
    gwir_provision_wipe(&p);for(size_t i=0;i<sizeof(p);++i) assert(((uint8_t*)&p)[i]==0);
}
int main(void){test_parse();test_apply();puts("PASS: APConfig form strict decoding, credentials staged/verified/committed, rollback on errors");}
