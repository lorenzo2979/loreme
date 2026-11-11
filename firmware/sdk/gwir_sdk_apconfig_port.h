#ifndef GWIR_SDK_APCONFIG_PORT_H
#define GWIR_SDK_APCONFIG_PORT_H
/* REV14: deliberately not commissioned to hardware. */
int gwir_sdk_button_read_pb23(int *pressed);
int gwir_sdk_apconfig_start(void *ctx);
int gwir_sdk_apconfig_stop(void *ctx);
#endif
