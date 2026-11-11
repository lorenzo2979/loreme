#include "gwir_sdk_apconfig_wifi.h"
#include "wifi_conf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned modes, ap_calls;
static int fail_mode, fail_ap;
int wifi_set_mode(rtw_mode_t mode) {
    ++modes;
    assert(mode==RTW_MODE_STA_AP || mode==RTW_MODE_STA);
    return fail_mode ? -1 : 0;
}
int wifi_start_ap(char *ssid, rtw_security_t security, char *password,
                  int ssid_len, int pass_len, int channel) {
    ++ap_calls;
    assert(security==RTW_SECURITY_OPEN);
    assert(password==NULL && pass_len==0); /* OPEN: never requires a secret */
    assert(ssid_len==(int)strlen(ssid));
    assert(channel==6);
    return fail_ap ? -1 : 0;
}
int main(void) {
    gwir_apcfg_identity id={{0},6u};
    assert(gwir_sdk_ap_wifi_enter(NULL,NULL)==-1);
    assert(modes==0);
    strcpy(id.ssid,"GW006-test");
    assert(gwir_sdk_ap_wifi_enter(NULL,&id)==0);
    assert(modes==1 && ap_calls==1);
    assert(gwir_sdk_ap_wifi_leave(NULL)==0);
    assert(modes==2);
    fail_mode=1;assert(gwir_sdk_ap_wifi_enter(NULL,&id)==-3);
    assert(ap_calls==1);
    fail_mode=0;fail_ap=1;assert(gwir_sdk_ap_wifi_enter(NULL,&id)==-4);
    id.channel=14;assert(gwir_sdk_ap_wifi_enter(NULL,&id)==-2);
    id.channel=6;
    memset(id.ssid,'X',sizeof(id.ssid));
    assert(gwir_sdk_ap_wifi_enter(NULL,&id)==-2);
    puts("PASS: AmebaD open APConfig; RTW_SECURITY_OPEN, NULL password, 0 length, rollback paths");
    return 0;
}
