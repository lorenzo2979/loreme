#include "gwir_amebad_tx_vendor.h"
#include "ameba_soc.h" /* Official AmebaD SDK, NOT distributed in this archive. */

static uint32_t vendor_free(void *unused)
{
    (void)unused;
    return IR_GetTxFIFOFreeLen(IR_DEV);
}
static int vendor_write(void *unused, const uint32_t *words,
                        uint32_t count, int last)
{
    (void)unused;
    /* IR_SendBuf takes a non-const pointer but only reads its contents. */
    IR_SendBuf(IR_DEV, (u32 *)(void *)words, count, last ? TRUE : FALSE);
    return 0;
}
static int vendor_start(void *unused)
{
    (void)unused;
    IR_Cmd(IR_DEV, IR_MODE_TX, ENABLE);
    return 0;
}
static int vendor_running(void *unused)
{
    (void)unused;
    return IR_FSMRunning(IR_DEV) ? 1 : 0;
}
gwir_amebad_tx_ops gwir_amebad_vendor_tx_ops(void)
{
    gwir_amebad_tx_ops ops;
    ops.ctx = NULL;
    ops.free_slots = vendor_free;
    ops.write_words = vendor_write;
    ops.start_tx = vendor_start;
    ops.is_running = vendor_running;
    return ops;
}
