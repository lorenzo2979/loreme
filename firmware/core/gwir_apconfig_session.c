#include "gwir_apconfig_session.h"
#include <stddef.h>

static void wipe(volatile void *ptr, size_t n)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (n--) *p++ = 0u;
}

static int valid(const gwir_apcfg_identity *id)
{
    size_t i, len = 0u;
    if (!id) return 0;
    while (len <= GWIR_APCFG_SSID_MAX && id->ssid[len] != 0) ++len;
    if (len == 0u || len > GWIR_APCFG_SSID_MAX) return 0;
    for (i = 0; i < len; ++i)
        if ((unsigned char)id->ssid[i] < 33u ||
            (unsigned char)id->ssid[i] > 126u) return 0;
    return id->channel >= 1u && id->channel <= 11u;
}

int gwir_apcfg_session_init(gwir_apcfg_session *s, gwir_apcfg_backend ops)
{
    if (!s || !ops.load_identity || !ops.wifi_enter || !ops.wifi_leave ||
        !ops.provisioner_start || !ops.provisioner_stop) return -1;
    *s = (gwir_apcfg_session){0};
    s->ops = ops;
    return 0;
}

int gwir_apcfg_session_stop(gwir_apcfg_session *s)
{
    int failures = 0;
    if (!s || !s->ops.wifi_leave || !s->ops.provisioner_stop) return -1;
    if (s->provisioner_started) {
        if (s->ops.provisioner_stop(s->ops.ctx) == 0)
            s->provisioner_started = 0u;
        else ++failures;
    }
    if (s->wifi_started) {
        if (s->ops.wifi_leave(s->ops.ctx) == 0)
            s->wifi_started = 0u;
        else ++failures;
    }
    return failures ? -2 : 0;
}

int gwir_apcfg_session_start(gwir_apcfg_session *s)
{
    gwir_apcfg_identity id = {{0}, 0};
    int ok;
    if (!s || !s->ops.load_identity || !s->ops.wifi_enter ||
        !s->ops.provisioner_start || !s->ops.wifi_leave) return -1;
    /* Fail closed on a previous partial teardown. A fresh button press
       cannot silently create another AP or leak a listener. */
    if (s->wifi_started || s->provisioner_started) return -2;
    ok = s->ops.load_identity(s->ops.ctx, &id);
    if (ok != 0 || !valid(&id)) { wipe(&id, sizeof(id)); return -3; }
    ok = s->ops.wifi_enter(s->ops.ctx, &id);
    wipe(&id, sizeof(id));
    if (ok != 0) {
        /* A vendor call can fail after partially activating Wi-Fi. Always
           request cleanup and keep the state marked if rollback fails. */
        s->wifi_started = 1u;
        if (gwir_apcfg_session_stop(s) != 0) return -5;
        return -4;
    }
    s->wifi_started = 1u;
    if (s->ops.provisioner_start(s->ops.ctx) != 0) {
        /* A listener may be partially open even when initialization fails. */
        s->provisioner_started = 1u;
        if (gwir_apcfg_session_stop(s) != 0) return -5;
        return -4;
    }
    s->provisioner_started = 1u;
    return 0;
}

int gwir_apcfg_session_is_active(const gwir_apcfg_session *s)
{
    return s && s->wifi_started && s->provisioner_started ? 1 : 0;
}

int gwir_apcfg_session_needs_cleanup(const gwir_apcfg_session *s)
{
    return s && (s->wifi_started || s->provisioner_started) ? 1 : 0;
}
