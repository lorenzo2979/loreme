#ifndef GWIR_APCONFIG_SESSION_H
#define GWIR_APCONFIG_SESSION_H
#include <stdint.h>

/* User requirement: APConfig is an OPEN Wi-Fi access point with NO password.
   This identity contains only a public SSID and channel, no secret. */
#define GWIR_APCFG_SSID_MAX 32u
typedef struct {
    char ssid[GWIR_APCFG_SSID_MAX + 1u];
    uint8_t channel;
} gwir_apcfg_identity;

typedef struct {
    void *ctx;
    /* Load public AP name and channel only. Station Wi-Fi credentials are
       separate and MUST NOT be overwritten by starting/stopping APConfig. */
    int (*load_identity)(void *ctx, gwir_apcfg_identity *out);
    /* Starts OPEN AP, no WPA/password (RTW_SECURITY_OPEN, NULL, length 0). */
    int (*wifi_enter)(void *ctx, const gwir_apcfg_identity *identity);
    /* Start a local provisioning listener restricted to AP interface.
       User requests no login/password/token on APConfig. The real HTTP/DHCP
       handler is not yet installed and must be implemented separately. */
    int (*provisioner_start)(void *ctx);
    int (*provisioner_stop)(void *ctx);
    /* Restore normal STA Wi-Fi; do not erase saved credentials. */
    int (*wifi_leave)(void *ctx);
} gwir_apcfg_backend;

typedef struct {
    gwir_apcfg_backend ops;
    uint8_t wifi_started;
    uint8_t provisioner_started;
} gwir_apcfg_session;

int gwir_apcfg_session_init(gwir_apcfg_session *s, gwir_apcfg_backend ops);
/* Start success requires AP AND provisioning listener. On partial failure,
   cleanup occurs; no AP is enabled during firmware startup. */
int gwir_apcfg_session_start(gwir_apcfg_session *s);
int gwir_apcfg_session_stop(gwir_apcfg_session *s);
int gwir_apcfg_session_is_active(const gwir_apcfg_session *s);
int gwir_apcfg_session_needs_cleanup(const gwir_apcfg_session *s);
#endif
