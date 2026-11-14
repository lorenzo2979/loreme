#ifndef GWIR_SDK_APCONFIG_PORT_H
#define GWIR_SDK_APCONFIG_PORT_H
#include "../core/gwir_apconfig_session.h"

/* Not self-commissioning: a separately reviewed physical GW006 board port
 * must supply BOTH approval and AP-only provisioning callbacks.
 * A missing hook means APConfig stays entirely OFF. */
int gwir_sdk_button_read_pb23(int *pressed);
int gwir_sdk_apconfig_install_backend(gwir_apcfg_backend ops);

/* Register a fail-closed 50ms button polling FreeRTOS task.
 * Returns -1/-2 without creating a task if board is not approved.
 * Task creation alone never opens Wi-Fi or touches NOR. */
int gwir_sdk_apconfig_commission(void);

/* Explicit programmatic AP start is disabled: use only physical long press. */
int gwir_sdk_apconfig_start(void *ctx);
/* Stop remains available after loss of board approval. */
int gwir_sdk_apconfig_stop(void *ctx);
/* Call only after successfully committed provisioning. */
int gwir_sdk_apconfig_committed(void);
#endif
