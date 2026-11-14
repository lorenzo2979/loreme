#include "gwir_apconfig_provision.h"
#include <stdint.h>
#include <stddef.h>

void gwir_provision_wipe(gwir_provision_profile *profile)
{
    if (!profile) return;
    volatile uint8_t *p = (volatile uint8_t *)profile;
    for (size_t i = 0; i < sizeof(*profile); ++i) p[i] = 0u;
}

static int hex_nibble(uint8_t c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (int)(c - 'A' + 10);
    return -1;
}

/* Decodes a single urlencoded field value, without ever reading outside len. */
static int decode_value(const uint8_t *s, size_t len, char *dst, size_t cap,
                        int password)
{
    size_t out = 0u;
    if (!dst || cap == 0u) return -1;
    for (size_t i = 0; i < len; ++i) {
        uint8_t c = s[i];
        if (c == '+') c = ' ';
        else if (c == '%') {
            if (i + 2u >= len) return -1;
            int hi = hex_nibble(s[i + 1u]), lo = hex_nibble(s[i + 2u]);
            if (hi < 0 || lo < 0) return -1;
            c = (uint8_t)((hi << 4) | lo);
            i += 2u;
        }
        if (c < 0x20u || c == 0x7fu || (password && c > 0x7eu)) return -1;
        if (out + 1u >= cap) return -1;
        dst[out++] = (char)c;
    }
    dst[out] = 0;
    return (int)out;
}

/* Strict dotted-decimal IPv4 validation; return network-order 32-bit bits. */
static int ipv4_number(const char *str, uint32_t *out)
{
    uint32_t value = 0u;
    unsigned octets = 0u;
    const char *p = str;
    if (!str || !out || !*str) return -1;
    for (;;) {
        unsigned n = 0u, digits = 0u;
        if (*p < '0' || *p > '9') return -1;
        if (*p == '0' && p[1] >= '0' && p[1] <= '9') return -1;
        while (*p >= '0' && *p <= '9') {
            n = n * 10u + (unsigned)(*p++ - '0');
            if (++digits > 3u || n > 255u) return -1;
        }
        if (octets++ >= 4u) return -1;
        value = (value << 8) | n;
        if (*p == 0) break;
        if (*p++ != '.') return -1;
        if (octets >= 4u) return -1;
    }
    if (octets != 4u) return -1;
    *out = value;
    return 0;
}

static int usable_ipv4(uint32_t a)
{
    const unsigned first = (unsigned)(a >> 24);
    return a != 0u && a != 0xffffffffu &&
           first > 0u && first < 224u && first != 127u &&
           /* APConfig uses 192.168.4.0/24 while active. */
           (a & 0xffffff00u) != 0xc0a80400u;
}

static int validate_network(const gwir_provision_profile *p)
{
    uint32_t ip, mask, gw, dns;
    if (p->use_dhcp) {
        return (!p->ipv4[0] && !p->netmask[0] && !p->gateway[0] &&
                !p->dns1[0] && !p->dns2[0]) ? 0 : -1;
    }
    if (ipv4_number(p->ipv4, &ip) || !usable_ipv4(ip) ||
        ipv4_number(p->netmask, &mask) ||
        ipv4_number(p->gateway, &gw) || !usable_ipv4(gw) ||
        ipv4_number(p->dns1, &dns) || !usable_ipv4(dns))
        return -1;
    if (p->dns2[0] && (ipv4_number(p->dns2, &dns) ||
                       !usable_ipv4(dns))) return -1;
    /* Contiguous IPv4 netmask, non-/0, non-/31, non-/32. */
    if (!mask || (mask & 0x3u) == 0x3u ||
        (((~mask) & ((~mask) + 1u)) != 0u)) return -1;
    const uint32_t host = ip & ~mask;
    const uint32_t gateway_host = gw & ~mask;
    if ((ip & mask) != (gw & mask) || ip == gw ||
        !host || host == ~mask ||
        !gateway_host || gateway_host == ~mask)
        return -1;
    return 0;
}

static int validate_duckdns(const gwir_provision_profile *p)
{
    static const char suffix[] = ".duckdns.org";
    size_t n = 0u;
    if (!p->duckdns_enabled)
        return (!p->duckdns_host[0] && !p->duckdns_token[0]) ? 0 : -1;
    if (!p->secure_transport_verified) return -1;
    while (n < sizeof(p->duckdns_host) && p->duckdns_host[n]) ++n;
    if (n >= sizeof(p->duckdns_host) || n < sizeof(suffix) ||
        n > 63u + sizeof(suffix) - 1u)
        return -1;
    const size_t label = n - (sizeof(suffix) - 1u);
    for (size_t i = 0u; i < sizeof(suffix) - 1u; ++i)
        if (p->duckdns_host[label + i] != suffix[i]) return -1;
    if (p->duckdns_host[0] == '-' || p->duckdns_host[label - 1u] == '-')
        return -1;
    for (size_t i = 0u; i < label; ++i) {
        const char c = p->duckdns_host[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-')) return -1;
    }
    size_t token = 0u;
    while (token < sizeof(p->duckdns_token) && p->duckdns_token[token]) {
        const char c = p->duckdns_token[token++];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-')) return -1;
    }
    return (token >= 8u && token < sizeof(p->duckdns_token)) ? 0 : -1;
}

static int matches(const uint8_t *p, size_t n, const char *key)
{
    size_t k = 0u;
    while (key[k]) ++k;
    if (n != k) return 0;
    for (size_t i = 0u; i < n; ++i)
        if (p[i] != (uint8_t)key[i]) return 0;
    return 1;
}

int gwir_provision_parse_form_secure(const uint8_t *body, size_t length,
                                    gwir_provision_profile *out,
                                    int trusted_secret_channel)
{
    enum { K_SSID, K_PASS, K_MODE, K_IP, K_MASK, K_GW,
           K_DNS1, K_DNS2, K_DUCK, K_HOST, K_TOKEN };
    static const char * const keys[] = {
        "ssid", "password", "ip_mode", "ip", "mask", "gateway",
        "dns1", "dns2", "duckdns", "duckdns_host", "duckdns_token"
    };
    unsigned seen = 0u;
    size_t pos = 0u;
    int secret_length = -1;
    if (!out) return -1;
    gwir_provision_wipe(out);
    out->use_dhcp = 1u;
    if (!body || length < 5u || length > GWIR_PROVISION_BODY_MAX)
        goto fail;
    while (pos < length) {
        const size_t begin = pos;
        while (pos < length && body[pos] != '&') ++pos;
        const size_t end = pos;
        size_t eq = begin;
        while (eq < end && body[eq] != '=') ++eq;
        if (eq == begin || eq == end) goto fail;
        unsigned key = 0u;
        for (; key < sizeof(keys)/sizeof(keys[0]); ++key)
            if (matches(body+begin, eq-begin, keys[key])) break;
        if (key >= sizeof(keys)/sizeof(keys[0]) ||
            (seen & (1u << key))) goto fail;
        seen |= 1u << key;
        const uint8_t *v = body + eq + 1u;
        const size_t vlen = end - eq - 1u;
        int decoded;
        switch (key) {
        case K_SSID:
            decoded = decode_value(v,vlen,out->ssid,sizeof(out->ssid),0);
            if (decoded < 1) goto fail;
            break;
        case K_PASS:
            decoded = decode_value(v,vlen,out->password,sizeof(out->password),1);
            if (decoded < 0 || (decoded != 0 && decoded < 8)) goto fail;
            /* An OPEN AP + HTTP does not protect the HOME Wi-Fi password.
             * Refuse any nonempty secret unless the transport has verified
             * confidentiality AND device identity out of band. A form flag
             * never decides trusted_secret_channel. */
            if (decoded != 0 && !trusted_secret_channel) goto fail;
            out->is_open = (uint8_t)(decoded == 0);
            break;
        case K_MODE:
            if (vlen == 4u && matches(v,vlen,"dhcp")) out->use_dhcp = 1u;
            else if (vlen == 6u && matches(v,vlen,"static")) out->use_dhcp = 0u;
            else goto fail;
            break;
        case K_IP:
            if (decode_value(v,vlen,out->ipv4,sizeof(out->ipv4),0) < 1) goto fail;
            break;
        case K_MASK:
            if (decode_value(v,vlen,out->netmask,sizeof(out->netmask),0) < 1) goto fail;
            break;
        case K_GW:
            if (decode_value(v,vlen,out->gateway,sizeof(out->gateway),0) < 1) goto fail;
            break;
        case K_DNS1:
            if (decode_value(v,vlen,out->dns1,sizeof(out->dns1),0) < 1) goto fail;
            break;
        case K_DNS2:
            if (decode_value(v,vlen,out->dns2,sizeof(out->dns2),0) < 1) goto fail;
            break;
        case K_DUCK:
            if (matches(v,vlen,"0")) out->duckdns_enabled = 0u;
            else if (matches(v,vlen,"1")) out->duckdns_enabled = 1u;
            else goto fail;
            break;
        case K_HOST:
            if (decode_value(v,vlen,out->duckdns_host,
                             sizeof(out->duckdns_host),0) < 1) goto fail;
            break;
        case K_TOKEN:
            if (!trusted_secret_channel) goto fail;
            secret_length = decode_value(v,vlen,out->duckdns_token,
                                         sizeof(out->duckdns_token),1);
            if (secret_length < 0) goto fail;
            break;
        default: goto fail;
        }
        if (pos == length) break;
        if (++pos == length) goto fail;
    }
    if (!(seen & (1u << K_SSID)) || !(seen & (1u << K_PASS)))
        goto fail;
    out->secure_transport_verified = (uint8_t)(trusted_secret_channel != 0);
    if (validate_network(out) || validate_duckdns(out)) goto fail;
    return 0;
fail:
    gwir_provision_wipe(out);
    return -3;
}

int gwir_provision_parse_form(const uint8_t *body, size_t length,
                              gwir_provision_profile *out)
{
    /* The legacy APConfig HTTP transport is not trusted to carry a DDNS
     * token. No HTML field can override this decision. */
    return gwir_provision_parse_form_secure(body, length, out, 0);
}

/* The public apply API may receive a user-created struct, not necessarily
 * a parser output. Verify all strings are terminated BEFORE parsing them.
 * This prevents a malformed caller from walking beyond fixed-size fields. */
static int zero_terminated(const char *value, size_t capacity)
{
    for (size_t i = 0u; i < capacity; ++i)
        if (!value[i]) return 1;
    return 0;
}

static int profile_storage_safe(const gwir_provision_profile *p)
{
    if (p->use_dhcp > 1u || p->is_open > 1u ||
        p->duckdns_enabled > 1u || p->secure_transport_verified > 1u)
        return 0;
    return zero_terminated(p->ssid, sizeof(p->ssid)) &&
           zero_terminated(p->password, sizeof(p->password)) &&
           zero_terminated(p->ipv4, sizeof(p->ipv4)) &&
           zero_terminated(p->netmask, sizeof(p->netmask)) &&
           zero_terminated(p->gateway, sizeof(p->gateway)) &&
           zero_terminated(p->dns1, sizeof(p->dns1)) &&
           zero_terminated(p->dns2, sizeof(p->dns2)) &&
           zero_terminated(p->duckdns_host, sizeof(p->duckdns_host)) &&
           zero_terminated(p->duckdns_token, sizeof(p->duckdns_token));
}

int gwir_provision_apply(const gwir_provision_profile *profile,
                          const gwir_provision_ops *ops)
{
    if (!profile || !ops || !ops->stage || !ops->verify ||
        !ops->commit || !ops->rollback) return -1;
    if (!profile_storage_safe(profile))
        return -2;
    /* Also defend callers that bypass the HTML parser and construct a
     * profile directly. A nonempty home Wi-Fi password needs a verified
     * protected channel, exactly as a DuckDNS token does. */
    if (profile->password[0] && !profile->secure_transport_verified)
        return -2;
    if (validate_network(profile) || validate_duckdns(profile))
        return -2;
    if (profile->ssid[0] == 0 || profile->ssid[GWIR_PROVISION_SSID_MAX] != 0 ||
        profile->password[GWIR_PROVISION_PASSWORD_MAX] != 0) return -2;
    if (ops->stage(ops->ctx, profile) != 0) {
        ops->rollback(ops->ctx);
        return -3;
    }
    if (ops->verify(ops->ctx) != 0) {
        ops->rollback(ops->ctx);
        return -4;
    }
    if (ops->commit(ops->ctx) != 0) {
        ops->rollback(ops->ctx);
        return -5;
    }
    return 0;
}
