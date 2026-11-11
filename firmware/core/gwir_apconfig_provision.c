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

int gwir_provision_parse_form(const uint8_t *body, size_t length,
                              gwir_provision_profile *out)
{
    int seen_ssid = 0, seen_password = 0, password_length = -1;
    if (!out) return -1;
    gwir_provision_wipe(out);
    if (!body || length < 5u || length > GWIR_PROVISION_BODY_MAX) return -2;
    size_t p = 0u;
    while (p < length) {
        size_t start = p, eq;
        while (p < length && body[p] != '&') ++p;
        const size_t end = p;
        eq = start;
        while (eq < end && body[eq] != '=') ++eq;
        if (eq == end || eq == start) goto fail;
        if (eq - start == 4u && body[start]=='s' && body[start+1u]=='s' &&
            body[start+2u]=='i' && body[start+3u]=='d') {
            if (seen_ssid++) goto fail;
            if (decode_value(body+eq+1u,end-eq-1u,out->ssid,sizeof(out->ssid),0) <= 0) goto fail;
        } else if (eq-start == 8u &&
                   body[start]=='p' && body[start+1u]=='a' && body[start+2u]=='s' &&
                   body[start+3u]=='s' && body[start+4u]=='w' && body[start+5u]=='o' &&
                   body[start+6u]=='r' && body[start+7u]=='d') {
            if (seen_password++) goto fail;
            password_length = decode_value(body+eq+1u,end-eq-1u,
                                          out->password,sizeof(out->password),1);
            if (password_length < 0) goto fail;
        } else goto fail;
        if (p == length) break;
        ++p;
        if (p == length) goto fail;
    }
    if (!seen_ssid || !seen_password) goto fail;
    if (password_length > 0 && password_length < 8) goto fail;
    out->is_open = (uint8_t)(password_length == 0);
    return 0;
fail:
    gwir_provision_wipe(out);
    return -3;
}

int gwir_provision_apply(const gwir_provision_profile *profile,
                          const gwir_provision_ops *ops)
{
    if (!profile || !ops || !ops->stage || !ops->verify ||
        !ops->commit || !ops->rollback) return -1;
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
