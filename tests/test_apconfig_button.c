#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "gwir_apconfig_button.h"

typedef struct { unsigned starts, stops; int fail_start, fail_stop; } fake;
static int start(void *ctx) {
    fake *f = (fake*)ctx; f->starts++;
    return f->fail_start ? -1 : 0;
}
static int stop(void *ctx) {
    fake *f = (fake*)ctx; f->stops++;
    return f->fail_stop ? -1 : 0;
}
static void init(gwir_apconfig_button *b, fake *f, uint32_t base) {
    gwir_apconfig_ops ops={f,start,stop};
    assert(gwir_apconfig_init(b,ops)==0);
    assert(gwir_apconfig_poll(b,base,0)==0);
}
static void long_press(gwir_apconfig_button *b,uint32_t t) {
    assert(gwir_apconfig_poll(b,t,1)==0);
    assert(gwir_apconfig_poll(b,t+49u,1)==0);
    assert(gwir_apconfig_poll(b,t+50u,1)==0);
    assert(gwir_apconfig_poll(b,t+5049u,1)==0);
    assert(gwir_apconfig_poll(b,t+5050u,1)==1);
}
static void release(gwir_apconfig_button *b,uint32_t t) {
    assert(gwir_apconfig_poll(b,t,0)==0);
    assert(gwir_apconfig_poll(b,t+49u,0)==0);
    assert(gwir_apconfig_poll(b,t+50u,0)==0);
}
static void test_short_press_and_bounce(void) {
    fake f={0};gwir_apconfig_button b;init(&b,&f,0);
    assert(gwir_apconfig_poll(&b,100,1)==0);
    assert(gwir_apconfig_poll(&b,120,0)==0);
    assert(gwir_apconfig_poll(&b,140,1)==0);
    assert(gwir_apconfig_poll(&b,180,1)==0);
    assert(gwir_apconfig_poll(&b,190,1)==0);
    assert(gwir_apconfig_poll(&b,5190,1)==1);
    assert(gwir_apconfig_poll(&b,6000,1)==0);
    assert(f.starts==1 && gwir_apconfig_is_active(&b)==1);
    release(&b,6200);
    assert(gwir_apconfig_poll(&b,5190u+300000u-1u,0)==0);
    assert(gwir_apconfig_poll(&b,5190u+300000u,0)==2);
    assert(!gwir_apconfig_is_active(&b) && f.stops==1);
    long_press(&b,310000);
    assert(f.starts==2);
    assert(gwir_apconfig_credentials_saved(&b)==2);
    assert(!gwir_apconfig_is_active(&b) && f.stops==2);
    assert(gwir_apconfig_credentials_saved(&b)==0);
}
static void test_failed_start_needs_release(void) {
    fake f={0};gwir_apconfig_button b;init(&b,&f,0);
    f.fail_start=1;
    assert(gwir_apconfig_poll(&b,50,1)==0);
    assert(gwir_apconfig_poll(&b,100,1)==0);
    assert(gwir_apconfig_poll(&b,5100,1)==-2);
    assert(!gwir_apconfig_is_active(&b) && f.starts==1);
    f.fail_start=0;
    assert(gwir_apconfig_poll(&b,20000,1)==0 && f.starts==1);
    release(&b,20050);
    long_press(&b,21000);
    assert(f.starts==2);
}
static void test_stop_failure_retries(void) {
    fake f={0};gwir_apconfig_button b;init(&b,&f,0);
    long_press(&b,1000);
    f.fail_stop=1;
    assert(gwir_apconfig_poll(&b,306050,1)==-3);
    assert(gwir_apconfig_is_active(&b)==1 && f.stops==1);
    assert(gwir_apconfig_poll(&b,306100,1)==-3 && f.stops==2);
    f.fail_stop=0;
    assert(gwir_apconfig_poll(&b,306150,1)==2);
    assert(gwir_apconfig_is_active(&b)==0 && f.stops==3);
}
static void test_initial_hold_ignored_and_wrap(void) {
    fake f={0};gwir_apconfig_button b;gwir_apconfig_ops ops={&f,start,stop};
    assert(gwir_apconfig_init(&b,ops)==0);
    assert(gwir_apconfig_poll(&b,UINT32_MAX-6000u,1)==0);
    assert(gwir_apconfig_poll(&b,UINT32_MAX-200u,1)==0);
    assert(f.starts==0);
    release(&b,UINT32_MAX-100u);
    long_press(&b,UINT32_MAX-20u);
    assert(f.starts==1);
    assert(gwir_apconfig_poll(&b,(UINT32_MAX-20u)+5050u+300000u,1)==2);
}
static void test_invalid_arguments(void) {
    gwir_apconfig_button b;gwir_apconfig_ops empty={0};fake f={0};
    assert(gwir_apconfig_init(NULL,empty)==-1);
    assert(gwir_apconfig_init(&b,empty)==-1);
    init(&b,&f,0);
    assert(gwir_apconfig_poll(NULL,0,0)==-1);
    assert(gwir_apconfig_poll(&b,0,2)==-1);
    assert(gwir_apconfig_credentials_saved(NULL)==-1);
    assert(gwir_apconfig_is_active(NULL)==0);
}
int main(void) {
    test_short_press_and_bounce();
    test_failed_start_needs_release();
    test_stop_failure_retries();
    test_initial_hold_ignored_and_wrap();
    test_invalid_arguments();
    puts("PASS: REV14 APConfig button debounce, hold, timeout, backend failures, boot hold, clock wrap");
    return 0;
}
