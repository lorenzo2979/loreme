/* GW018-DM AmebaD reference boot integration of the GW006 IR long-frame
 * engine. The app boots and initializes the real GWIR core, RX capture worker,
 * half-duplex vendor IR bus and protocol locally. This is NOT a GW006 release:
 * board pinmux/flash and network authentication remain disabled.
 * Never forward untrusted TCP bytes directly into this diagnostic worker. */
#include "ameba_soc.h"
#include "../core/gwir_engine.h"
#include "../core/gwir_protocol.h"
#include "../realtek/gwir_amebad_bus.h"
#include "../realtek/gwir_amebad_vendor_bus.h"
#include "../realtek/gwir_amebad_rx_fifo.h"
#include "../realtek/gwir_amebad_rx_irq.h"
#include <stdint.h>

static gwir_engine ir_engine;                 /* ~50KiB, never on task stack */
static gwir_amebad_bus ir_bus;               /* ~8KiB */
static gwir_amebad_rx_queue ir_irq_queue;    /* ~4KiB */
static gwir_amebad_rx_worker ir_worker;      /* ~8KiB */
static gwir_amebad_rx_irq_stats ir_stats;

static void irq_lock(void *unused) { (void)unused; InterruptDis(IR_IRQ); }
static void irq_unlock(void *unused) { (void)unused; InterruptEn(IR_IRQ, 10); }

/* These callbacks operate in a serialized worker context only. They never
 * bypass gwir_amebad_bus board_ir_ready() and fail closed without approval. */
static int hw_arm(void *unused)
{
    (void)unused;
    return gwir_amebad_bus_start_rx(&ir_bus);
}
static int hw_disarm(void *unused)
{
    (void)unused;
    return gwir_amebad_bus_stop_rx(&ir_bus);
}
static int hw_tx(void *unused, uint32_t carrier, uint16_t duty,
                 const uint32_t *durations, uint16_t count, uint32_t gap)
{
    (void)unused;
    uint32_t budget = 120000u;
    if (gwir_amebad_bus_start_tx(&ir_bus, durations, count, carrier, duty,
                                gap, budget) != 0) return -1;
    uint32_t start = xTaskGetTickCount();
    for (;;) {
        int status = gwir_amebad_bus_poll(&ir_bus);
        if (status == GWIR_BUS_FINISHED) return 0;
        if (status < 0 || (uint32_t)(xTaskGetTickCount()-start) >
            (budget / portTICK_PERIOD_MS) + 2u) {
            (void)gwir_amebad_bus_abort(&ir_bus);
            return -1;
        }
        vTaskDelay(1);
    }
}

/* Explicitly test the actual core protocol path on the KM4 boot task.
 * No IR pinmux/IRQ enable, no flash write and no network listener. */
static int local_protocol_probe(void)
{
    uint8_t request[GWIR_HEADER_SIZE];
    uint8_t response[GWIR_HEADER_SIZE + 8u];
    size_t request_size = 0u, response_size = 0u;
    gwir_message parsed;
    if (gwir_encode(request,sizeof(request),GWIR_HELLO,1u,
                    NULL,0u,&request_size) != 0) return -1;
    if (gwir_engine_process(&ir_engine,request,request_size,
                            response,sizeof(response),&response_size) != 0)
        return -2;
    if (gwir_decode(response,response_size,&parsed) != 0 ||
        parsed.command != GWIR_HELLO_REPLY ||
        parsed.payload_len != 8u ||
        parsed.payload[1] != GWIR_MAX_FRAMES_PER_BUTTON)
        return -3;
    return 0;
}

static void ir_worker_task(void *unused)
{
    (void)unused;
    for (;;) {
        /* IRQ queue -> capture builder -> engine.
         * Receiving cannot actually start without validated board config. */
        (void)gwir_amebad_rx_worker_step(&ir_worker, 64u);
        vTaskDelay(2);
    }
}

int gwir_gw018_ir_boot_prepare(void)
{
    gwir_amebad_bus_ops ops = gwir_amebad_vendor_bus_ops();
    gwir_ir_hal hal = {hw_tx,hw_arm,hw_disarm,NULL};
    gwir_amebad_rx_lock lock = {irq_lock,irq_unlock,NULL};
    gwir_amebad_rx_queue_init(&ir_irq_queue);
    if (gwir_amebad_bus_init(&ir_bus,&ops) != 0) return -1;
    gwir_engine_init(&ir_engine,hal);
    if (gwir_amebad_rx_worker_init(&ir_worker,&ir_irq_queue,&ir_engine,
                                   lock,10000000u,0u,38000u,330u,30000u) != 0)
        return -2;
    if (gwir_amebad_rx_irq_bind(&ir_irq_queue,30000u,&ir_stats) != 0)
        return -3;
    if (local_protocol_probe() != 0) return -4;
    /* A default GW018/WBRG1 image is not authorized for GW006 IR pins.
     * Do NOT start RX worker: its queue lock touches the hardware IRQ mask.
     * The driver code stays present in the native ELF for a future validated
     * board port to authorize and launch. */
    if (ops.board_ir_ready(ops.ctx) != 1) return 0;
    if (xTaskCreate(ir_worker_task, "GWIR-IR", 1024u, NULL,
                    tskIDLE_PRIORITY+2u,NULL) != pdPASS)
        return -5;
    return 0;
}
