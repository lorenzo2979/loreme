#include "gwir_apconfig_controller.h"
#include <stddef.h>
#include <string.h>

static int start_ap(void *ctx)
{
    gwir_apconfig_controller *c = (gwir_apconfig_controller *)ctx;
    if (!c || !c->initialized || gwir_apcfg_session_needs_cleanup(&c->session))
        return -1;
    return gwir_apcfg_session_start(&c->session);
}

static int stop_ap(void *ctx)
{
    gwir_apconfig_controller *c = (gwir_apconfig_controller *)ctx;
    if (!c || !c->initialized) return -1;
    if (gwir_apcfg_session_stop(&c->session) != 0) return -1;
    return gwir_apcfg_session_needs_cleanup(&c->session) ? -1 : 0;
}

int gwir_apconfig_controller_init(gwir_apconfig_controller *c,
                                 gwir_apcfg_backend backend,
                                 int (*read_pressed)(void *, int *),
                                 void *read_ctx)
{
    gwir_apconfig_ops button_ops;
    if (!c || !read_pressed) return -1;
    memset(c, 0, sizeof(*c));
    if (gwir_apcfg_session_init(&c->session, backend) != 0) return -2;
    c->read_pressed = read_pressed;
    c->read_ctx = read_ctx;
    button_ops.ctx = c;
    button_ops.ap_start = start_ap;
    button_ops.ap_stop = stop_ap;
    if (gwir_apconfig_init(&c->button, button_ops) != 0) return -3;
    c->initialized = 1u;
    return 0;
}

int gwir_apconfig_controller_stop(gwir_apconfig_controller *c)
{
    if (!c || !c->initialized) return -1;
    if (gwir_apconfig_is_active(&c->button)) {
        int r = gwir_apconfig_credentials_saved(&c->button);
        return r < 0 ? -2 : 0;
    }
    return gwir_apcfg_session_stop(&c->session) == 0 ? 0 : -2;
}

int gwir_apconfig_controller_poll(gwir_apconfig_controller *c, uint32_t now_ms)
{
    int pressed = 0;
    if (!c || !c->initialized || !c->read_pressed) return -1;
    /* If an earlier start failed halfway, finish rollback before allowing
       another long press. The button state machine cannot do this itself. */
    if (!gwir_apconfig_is_active(&c->button) &&
        gwir_apcfg_session_needs_cleanup(&c->session) &&
        gwir_apcfg_session_stop(&c->session) != 0) return -2;

    if (c->read_pressed(c->read_ctx, &pressed) != 0 ||
        (pressed != 0 && pressed != 1)) {
        /* A failed GPIO read must not leave an OPEN AP indefinitely. */
        return gwir_apconfig_controller_stop(c) == 0 ? -3 : -4;
    }
    return gwir_apconfig_poll(&c->button, now_ms, pressed);
}

int gwir_apconfig_controller_committed(gwir_apconfig_controller *c)
{
    return gwir_apconfig_controller_stop(c);
}
