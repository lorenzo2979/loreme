#include "gwir_apconfig_controller.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int pressed, read_fail, ap_fail, listen_fail, stop_fail;
    unsigned entered, left, listeners_started, listeners_stopped;
} test_board;

static int read_btn(void *p, int *pressed) {
    test_board *b=(test_board*)p;
    if (b->read_fail) return -1;
    *pressed=b->pressed;
    return 0;
}
static int load(void *p,gwir_apcfg_identity *id) {
    (void)p;
    strcpy(id->ssid, "GW006-IR-SETUP");
    id->channel=6u;
    return 0;
}
static int enter(void *p, const gwir_apcfg_identity *id) {
    test_board *b=(test_board*)p;
    assert(strcmp(id->ssid,"GW006-IR-SETUP")==0);
    b->entered++;
    return b->ap_fail ? -1 : 0;
}
static int leave(void *p) {
    test_board *b=(test_board*)p;
    b->left++;
    return b->stop_fail ? -1 : 0;
}
static int start_listener(void *p) {
    test_board *b=(test_board*)p;
    b->listeners_started++;
    return b->listen_fail ? -1 : 0;
}
static int stop_listener(void *p) {
    test_board *b=(test_board*)p;
    b->listeners_stopped++;
    return b->stop_fail ? -1 : 0;
}
static void init(gwir_apconfig_controller *c,test_board *b) {
    gwir_apcfg_backend o={b,load,enter,start_listener,stop_listener,leave};
    assert(gwir_apconfig_controller_init(c,o,read_btn,b)==0);
    assert(gwir_apconfig_controller_poll(c,0)==0);
}
static void press(gwir_apconfig_controller *c,test_board *b,uint32_t t) {
    b->pressed=1;
    assert(gwir_apconfig_controller_poll(c,t)==0);
    assert(gwir_apconfig_controller_poll(c,t+50)==0);
    assert(gwir_apconfig_controller_poll(c,t+5050)==1);
}
static void release(gwir_apconfig_controller *c,test_board *b,uint32_t t) {
    b->pressed=0;
    assert(gwir_apconfig_controller_poll(c,t)==0);
    assert(gwir_apconfig_controller_poll(c,t+50)==0);
}
static void full_window(void) {
    gwir_apconfig_controller c;
    test_board b={0};
    init(&c,&b);
    b.pressed=1;
    assert(gwir_apconfig_controller_poll(&c,100)==0);
    release(&c,&b,200); /* short tap never opens AP */
    assert(b.entered==0);
    press(&c,&b,1000);
    assert(b.entered==1 && b.listeners_started==1);
    assert(gwir_apcfg_session_is_active(&c.session));
    assert(gwir_apconfig_controller_poll(&c,306049)==0);
    assert(gwir_apconfig_controller_poll(&c,306050)==2);
    assert(!gwir_apcfg_session_needs_cleanup(&c.session));
    assert(b.left==1 && b.listeners_stopped==1);
    release(&c,&b,607000);
    press(&c,&b,608000);
    assert(gwir_apconfig_controller_committed(&c)==0);
    assert(b.left==2 && b.listeners_stopped==2);
    assert(gwir_apconfig_controller_stop(&c)==0);
}
static void failed_partial_start_cleanup(void) {
    gwir_apconfig_controller c;
    test_board b={0};
    init(&c,&b);
    b.listen_fail=1; b.stop_fail=1; b.pressed=1;
    assert(gwir_apconfig_controller_poll(&c,100)==0);
    assert(gwir_apconfig_controller_poll(&c,150)==0);
    assert(gwir_apconfig_controller_poll(&c,5150)==-2);
    assert(gwir_apcfg_session_needs_cleanup(&c.session));
    assert(gwir_apconfig_controller_poll(&c,5200)==-2); /* no re-open */
    b.stop_fail=0;
    assert(gwir_apconfig_controller_poll(&c,5300)==0);
    assert(!gwir_apcfg_session_needs_cleanup(&c.session));
    assert(b.entered==1);
}
static void gpio_fails_closes_ap(void) {
    gwir_apconfig_controller c;
    test_board b={0};
    init(&c,&b);
    press(&c,&b,1000);
    b.read_fail=1;
    assert(gwir_apconfig_controller_poll(&c,7000)==-3);
    assert(!gwir_apconfig_is_active(&c.button));
    assert(!gwir_apcfg_session_needs_cleanup(&c.session));
    assert(b.left==1);
}
static void gpio_fails_stop_retries(void) {
    gwir_apconfig_controller c;
    test_board b={0};
    init(&c,&b);
    press(&c,&b,1000);
    b.read_fail=b.stop_fail=1;
    assert(gwir_apconfig_controller_poll(&c,7000)==-4);
    assert(gwir_apconfig_is_active(&c.button));
    b.stop_fail=0;
    assert(gwir_apconfig_controller_poll(&c,7100)==-3);
    assert(!gwir_apconfig_is_active(&c.button));
}
static void refuses_null(void) {
    assert(gwir_apconfig_controller_init(NULL,(gwir_apcfg_backend){0},read_btn,NULL)==-1);
    assert(gwir_apconfig_controller_poll(NULL,0)==-1);
    assert(gwir_apconfig_controller_committed(NULL)==-1);
}
int main(void) {
    full_window();
    failed_partial_start_cleanup();
    gpio_fails_closes_ap();
    gpio_fails_stop_retries();
    refuses_null();
    puts("GW006_APCONFIG_CONTROLLER=PASS; NO_GPIO_NO_NETWORK_NO_FLASH");
    return 0;
}
