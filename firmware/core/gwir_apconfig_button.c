#include "gwir_apconfig_button.h"
#include <stddef.h>

int gwir_apconfig_init(gwir_apconfig_button *b, gwir_apconfig_ops ops)
{
    if (!b || !ops.ap_start || !ops.ap_stop) return -1;
    *b = (gwir_apconfig_button){0};
    b->ops = ops;
    return 0;
}

static int stop_if_pending(gwir_apconfig_button *b)
{
    if (!b->ap_active || !b->pending_stop) return 0;
    if (b->ops.ap_stop(b->ops.ctx) != 0) return -3;
    b->pending_stop = 0;
    b->ap_active = 0;
    return 2;
}

int gwir_apconfig_poll(gwir_apconfig_button *b, uint32_t now_ms, int pressed)
{
    if (!b || !b->ops.ap_start || !b->ops.ap_stop || (pressed != 0 && pressed != 1))
        return -1;

    /* A failed stop is retried; never announce that the AP closed unless it did. */
    if (b->ap_active &&
        (uint32_t)(now_ms - b->ap_since_ms) >= GWIR_APCONFIG_WINDOW_MS)
        b->pending_stop = 1u;
    int stop = stop_if_pending(b);
    if (stop < 0) return stop;

    if (!b->initialized) {
        b->initialized = 1u;
        b->sampled_pressed = (uint8_t)pressed;
        b->transition_ms = now_ms;
        /* Never interpret a button held during boot as a new action. */
        b->stable_pressed = (uint8_t)pressed;
        b->hold_fired = (uint8_t)pressed;
        return stop;
    }

    if ((uint8_t)pressed != b->sampled_pressed) {
        b->sampled_pressed = (uint8_t)pressed;
        b->transition_ms = now_ms;
    }
    if (b->stable_pressed != b->sampled_pressed &&
        (uint32_t)(now_ms - b->transition_ms) >= GWIR_APCONFIG_DEBOUNCE_MS) {
        b->stable_pressed = b->sampled_pressed;
        if (b->stable_pressed) {
            b->held_since_ms = now_ms;
            b->hold_fired = 0u;
        } else {
            b->hold_fired = 0u;
        }
    }
    if (b->stable_pressed && !b->hold_fired && !b->ap_active &&
        (uint32_t)(now_ms - b->held_since_ms) >= GWIR_APCONFIG_HOLD_MS) {
        b->hold_fired = 1u;
        if (b->ops.ap_start(b->ops.ctx) != 0) return -2;
        b->ap_since_ms = now_ms;
        b->ap_active = 1u;
        return 1;
    }
    return stop;
}

int gwir_apconfig_credentials_saved(gwir_apconfig_button *b)
{
    if (!b || !b->ops.ap_stop) return -1;
    if (!b->ap_active) return 0;
    b->pending_stop = 1u;
    return stop_if_pending(b);
}

int gwir_apconfig_is_active(const gwir_apconfig_button *b)
{
    return b && b->ap_active ? 1 : 0;
}
