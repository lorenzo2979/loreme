#ifndef GWIR_APCONFIG_PROVISION_H
#define GWIR_APCONFIG_PROVISION_H
#include <stddef.h>
#include <stdint.h>
/* An APConfig form arrives only through a future AP-interface-only HTTP server.
 * This parser does not listen on the network or change credentials on its own. */
#define GWIR_PROVISION_SSID_MAX 32u
#define GWIR_PROVISION_PASSWORD_MAX 63u
#define GWIR_PROVISION_BODY_MAX 512u
#define GWIR_PROVISION_TCP_PORT 42424u /* authenticated normal LAN service, NOT APConfig */
typedef struct {
    char ssid[GWIR_PROVISION_SSID_MAX + 1u];
    char password[GWIR_PROVISION_PASSWORD_MAX + 1u];
    uint8_t is_open; /* 1=home network open, 0=WPA2 passphrase; unrelated to open APConfig */
    uint8_t use_dhcp;  /* default = 1; static settings only apply to STA */
    char ipv4[16];
    char netmask[16];
    char gateway[16];
    char dns1[16];
    char dns2[16];
    uint8_t duckdns_enabled;
    uint8_t secure_transport_verified; /* required for accepting a DuckDNS secret */
    char duckdns_host[80];
    char duckdns_token[129];
} gwir_provision_profile;
typedef struct {
    void *ctx;
    /* Stage in RAM or a non-active slot. Never overwrite working credentials here. */
    int (*stage)(void *ctx, const gwir_provision_profile *profile);
    /* Verify association/connectivity before overwriting active profile. */
    int (*verify)(void *ctx);
    /* Atomic/rollback-safe commit of verified staged profile. */
    int (*commit)(void *ctx);
    /* Drop staged profile, never erase original working profile. */
    void (*rollback)(void *ctx);
} gwir_provision_ops;
/* Parse exactly 'ssid=...&password=...' (either order) from HTTP form body.
 * Rejects duplicates, unknown fields, malformed escapes and control chars;
 * password is optional empty only for an open home AP. On failure, output wiped. */
int gwir_provision_parse_form(const uint8_t *body, size_t length,
                              gwir_provision_profile *out);
/* Extended form with ip_mode=dhcp|static, ip, mask, gateway, dns1,
 * optional dns2, duckdns=0|1, duckdns_host, duckdns_token.
 * trusted_secret_channel MUST be verified by the transport, not supplied
 * by the browser/form. An unprotected open-AP HTTP session MUST pass 0.
 * DuckDNS tokens cannot enter the device over unprotected provisioning.
 * Existing legacy SSID/password parser remains for backwards compatibility.
 */
int gwir_provision_parse_form_secure(const uint8_t *body, size_t length,
                                    gwir_provision_profile *out,
                                    int trusted_secret_channel);
/* Return success only if stage, connectivity verification and commit succeed.
 * Does not turn on any radio, start web services, or log passwords. */
int gwir_provision_apply(const gwir_provision_profile *profile,
                          const gwir_provision_ops *ops);
void gwir_provision_wipe(gwir_provision_profile *profile);
#endif
