#ifndef GWIR_SDK_APCONFIG_WIFI_H
#define GWIR_SDK_APCONFIG_WIFI_H
#include "../core/gwir_apconfig_session.h"
/* SDK-backed AP transport implementation, always called from a normal task.
   Never called during startup in REV15: no board approval, DHCP and the AP-only
   provisioning listener are commissioned yet. */
int gwir_sdk_ap_wifi_enter(void *ctx, const gwir_apcfg_identity *identity);
int gwir_sdk_ap_wifi_leave(void *ctx);
#endif
