#ifndef GWIR_APCONFIG_CONTROLLER_H
#define GWIR_APCONFIG_CONTROLLER_H
#include <stdint.h>
#include "gwir_apconfig_button.h"
#include "gwir_apconfig_session.h"

/* APConfig board integration: physical PB23 sampling -> debounced hold ->
 * 5-minute open-AP provisioning session -> reliable cleanup.
 * Does not implement, enable, or approve GPIO, Wi-Fi, or an HTTP listener.
 * Board-specific code supplies only verified, non-ISR callbacks.
 */
typedef struct {
    gwir_apconfig_button button;
    gwir_apcfg_session session;
    int (*read_pressed)(void *ctx, int *pressed);
    void *read_ctx;
    uint8_t initialized;
} gwir_apconfig_controller;

/* No GPIO, AP or network activity occurs during init. */
int gwir_apconfig_controller_init(gwir_apconfig_controller *controller,
                                 gwir_apcfg_backend backend,
                                 int (*read_pressed)(void *, int *),
                                 void *read_ctx);

/* Called from a regular task with a monotonic millisecond tick.
 * Positive return: 1 AP opened, 2 AP closed. Negative: fail closed.
 */
int gwir_apconfig_controller_poll(gwir_apconfig_controller *controller,
                                 uint32_t now_ms);

/* Call after successful, durable configuration commit; closes AP. */
int gwir_apconfig_controller_committed(gwir_apconfig_controller *controller);

/* Explicit shutdown/retry; must succeed before device sleep or reboot. */
int gwir_apconfig_controller_stop(gwir_apconfig_controller *controller);
#endif
