#include "gwir_apconfig_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned load, enter, listen, unlisten, leave;
    int fail_load, fail_enter, fail_listen, fail_unlisten, fail_leave;
    int malformed;
} fixture;
static int load(void *ctx, gwir_apcfg_identity *i) {
    fixture *f=(fixture*)ctx; ++f->load;
    if (f->fail_load) return -1;
    strcpy(i->ssid,"GW006-test-unit");
    /* APConfig intentionally has no password. */
    i->channel=6u;
    if (f->malformed) { i->ssid[0]='\0'; }
    return 0;
}
static int enter(void *ctx, const gwir_apcfg_identity *i) {
    fixture *f=(fixture*)ctx; ++f->enter;
    assert(strcmp(i->ssid,"GW006-test-unit")==0);
    assert(i->channel==6u);
    return f->fail_enter ? -1 : 0;
}
static int listen(void *ctx) {
    fixture *f=(fixture*)ctx; ++f->listen;
    return f->fail_listen ? -1 : 0;
}
static int unlisten(void *ctx) {
    fixture *f=(fixture*)ctx; ++f->unlisten;
    return f->fail_unlisten ? -1 : 0;
}
static int leave(void *ctx) {
    fixture *f=(fixture*)ctx; ++f->leave;
    return f->fail_leave ? -1 : 0;
}
static gwir_apcfg_backend backend(fixture *f) {
    gwir_apcfg_backend b={f,load,enter,listen,unlisten,leave}; return b;
}
static void ok_case(void) {
    fixture f={0};gwir_apcfg_session s;
    assert(gwir_apcfg_session_init(&s,backend(&f))==0);
    assert(gwir_apcfg_session_start(&s)==0);
    assert(gwir_apcfg_session_is_active(&s));
    assert(gwir_apcfg_session_start(&s)==-2); /* no duplicate hotspot */
    assert(gwir_apcfg_session_stop(&s)==0);
    assert(!gwir_apcfg_session_needs_cleanup(&s));
    assert(gwir_apcfg_session_stop(&s)==0); /* idempotent */
    assert(f.load==1 && f.enter==1 && f.listen==1 && f.unlisten==1 && f.leave==1);
}
static void missing_ssid(void) {
    fixture f={0};gwir_apcfg_session s;
    f.malformed=1;
    assert(gwir_apcfg_session_init(&s,backend(&f))==0);
    assert(gwir_apcfg_session_start(&s)==-3);
    assert(f.enter==0 && f.listen==0);
    f.malformed=0;f.fail_load=1;
    assert(gwir_apcfg_session_start(&s)==-3);
    assert(f.enter==0);
}
static void ap_start_rollback(void) {
    fixture f={0};gwir_apcfg_session s;
    f.fail_enter=1;
    assert(gwir_apcfg_session_init(&s,backend(&f))==0);
    assert(gwir_apcfg_session_start(&s)==-4);
    assert(f.leave==1 && f.listen==0 && !gwir_apcfg_session_needs_cleanup(&s));
}
static void listener_start_rollback(void) {
    fixture f={0};gwir_apcfg_session s;
    f.fail_listen=1;
    assert(gwir_apcfg_session_init(&s,backend(&f))==0);
    assert(gwir_apcfg_session_start(&s)==-4);
    assert(f.unlisten==1 && f.leave==1 && !gwir_apcfg_session_needs_cleanup(&s));
}
static void failed_stop_retries(void) {
    fixture f={0};gwir_apcfg_session s;
    assert(gwir_apcfg_session_init(&s,backend(&f))==0);
    assert(gwir_apcfg_session_start(&s)==0);
    f.fail_leave=1;f.fail_unlisten=1;
    assert(gwir_apcfg_session_stop(&s)==-2);
    assert(gwir_apcfg_session_needs_cleanup(&s));
    assert(gwir_apcfg_session_start(&s)==-2);
    f.fail_leave=0;f.fail_unlisten=0;
    assert(gwir_apcfg_session_stop(&s)==0);
    assert(!gwir_apcfg_session_needs_cleanup(&s));
}
static void incomplete_start_cleanup_failure(void) {
    fixture f={0};gwir_apcfg_session s;
    f.fail_listen=1;f.fail_leave=1;
    assert(gwir_apcfg_session_init(&s,backend(&f))==0);
    assert(gwir_apcfg_session_start(&s)==-5);
    assert(gwir_apcfg_session_needs_cleanup(&s));
    assert(!gwir_apcfg_session_is_active(&s));
    f.fail_leave=0;
    assert(gwir_apcfg_session_stop(&s)==0);
}
int main(void) {
    ok_case();missing_ssid();ap_start_rollback();listener_start_rollback();
    failed_stop_retries();incomplete_start_cleanup_failure();
    puts("PASS: APConfig OPEN network, listener required, partial rollback and stop retry");
    return 0;
}
