#include "gwir_sdk_apconfig_wifi.h"
#include "wifi_conf.h" /* Official AmebaD Wi-Fi API */
#include <stddef.h>

/* OPEN APConfig (no WPA2, no password, no login token). Compiles and links
   against vendor SDK, but is not activated by REV15 startup. GPIO approval,
   AP-only HTTP provisioning, DHCP, and station restoration still pending. */
static size_t bounded_len(const char *s, size_t max)
{
    size_t n = 0u;
    while (n <= max && s[n] != 0) ++n;
    return n;
}

int gwir_sdk_ap_wifi_enter(void *ctx, const gwir_apcfg_identity *identity)
{
    size_t ssid_len;
    (void)ctx;
    if (!identity) return -1;
    ssid_len = bounded_len(identity->ssid, GWIR_APCFG_SSID_MAX);
    if (ssid_len < 1u || ssid_len > GWIR_APCFG_SSID_MAX ||
        identity->channel < 1u || identity->channel > 11u) return -2;
    /* Do not erase the current saved STA profile. Only a verified board
       integration is allowed to call this adapter during an APConfig window. */
    if (wifi_set_mode(RTW_MODE_STA_AP) < 0) return -3;
    if (wifi_start_ap((char *)identity->ssid, RTW_SECURITY_OPEN,
                      NULL, (int)ssid_len, 0, (int)identity->channel) < 0) return -4;
    return 0;
}

int gwir_sdk_ap_wifi_leave(void *ctx)
{
    (void)ctx;
    /* No wifi_stop_ap() in SDK API. STA transition must be board-tested. */
    return wifi_set_mode(RTW_MODE_STA) < 0 ? -1 : 0;
}
