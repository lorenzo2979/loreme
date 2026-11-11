#ifndef GWIR_APCONFIG_BUTTON_H
#define GWIR_APCONFIG_BUTTON_H
#include <stdint.h>

/* Drive from a regular app task, not the GPIO ISR. Timing is milliseconds. */
#define GWIR_APCONFIG_DEBOUNCE_MS 50u
#define GWIR_APCONFIG_HOLD_MS 5000u
#define GWIR_APCONFIG_WINDOW_MS 300000u

typedef struct {
    void *ctx;
    /* Return 0 only after securely starting a WPA2+ AP with device-unique
       non-hardcoded credentials and a bounded local provisioning service. */
    int (*ap_start)(void *ctx);
    /* Return 0 only after AP and its provisioning listeners are stopped. */
    int (*ap_stop)(void *ctx);
} gwir_apconfig_ops;

typedef struct {
    gwir_apconfig_ops ops;
    uint32_t transition_ms;
    uint32_t held_since_ms;
    uint32_t ap_since_ms;
    uint8_t initialized;
    uint8_t sampled_pressed;
    uint8_t stable_pressed;
    uint8_t hold_fired;
    uint8_t ap_active;
    uint8_t pending_stop;
} gwir_apconfig_button;

/* pressed: 1 physically pressed; 0 released. Returns 1 when AP opens,
 * 2 when AP closes, 0 for no change, negative if a backend fails. */
int gwir_apconfig_init(gwir_apconfig_button *b, gwir_apconfig_ops ops);
int gwir_apconfig_poll(gwir_apconfig_button *b, uint32_t now_ms, int pressed);
/* Call only after network credentials have been committed successfully.
 * Does not delete, reset or alter any saved credential. */
int gwir_apconfig_credentials_saved(gwir_apconfig_button *b);
int gwir_apconfig_is_active(const gwir_apconfig_button *b);
#endif
