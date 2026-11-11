#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/core/gwir_engine.h"
#include "../firmware/realtek/gwir_amebad_rx_fifo.h"
#include "../firmware/realtek/gwir_amebad_rx_vendor.h"
#include "ameba_soc.h"

IR_TypeDef gwir_ir_test_device;
static uint32_t hardware_fifo[32];
static uint32_t hardware_size;
static uint32_t hardware_read;
static unsigned lock_depth, locks, unlocks;
static void lock_irq(void *unused) { (void)unused;assert(lock_depth==0); lock_depth=1;locks++; }
static void unlock_irq(void *unused) { (void)unused;assert(lock_depth==1);lock_depth=0;unlocks++; }
static gwir_amebad_rx_lock lock_ops(void) {
    gwir_amebad_rx_lock l={lock_irq,unlock_irq,NULL};return l;
}
u32 IR_GetRxDataLen(IR_TypeDef *ir) { assert(ir==IR_DEV);return hardware_size-hardware_read; }
u32 IR_ReceiveData(IR_TypeDef *ir) { assert(ir==IR_DEV && hardware_read<hardware_size);return hardware_fifo[hardware_read++]; }
static uint32_t word_us(uint32_t us, int high) {
    uint32_t ticks=us*10u;
    assert(ticks<0x7fffffffu);
    return ticks | (high?0x80000000u:0u);
}
static void init(gwir_engine *e, gwir_amebad_rx_queue *q,gwir_amebad_rx_worker *w, int mark_level) {
    gwir_ir_hal h={0};
    gwir_engine_init(e,h);
    e->rx_armed=1;
    gwir_amebad_rx_queue_init(q);
    assert(gwir_amebad_rx_worker_init(w,q,e,lock_ops(),10000000u,(uint8_t)mark_level,38000u,333u,30000u)==0);
}
static void push_mark(gwir_amebad_rx_queue *q, uint32_t us, int mark_level) {
    assert(!gwir_amebad_rx_irq_word(q,word_us(us,mark_level)));
}
static void push_space(gwir_amebad_rx_queue *q, uint32_t us, int mark_level) {
    assert(!gwir_amebad_rx_irq_word(q,word_us(us,!mark_level)));
}
static void test_basic(int mark_level) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,mark_level);
    push_mark(&q,9000,mark_level);push_space(&q,4500,mark_level);
    push_mark(&q,560,mark_level);push_space(&q,560,mark_level);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,99u)==1);
    assert(e.rx_count==1 && e.rx_queue[0].count==4);
    assert(e.rx_queue[0].durations_us[0]==9000u);
    assert(e.rx_queue[0].durations_us[1]==4500u);
    assert(e.rx_queue[0].durations_us[2]==560u);
    assert(e.rx_queue[0].durations_us[3]==560u);
    assert(e.rx_queue[0].gap_after_us==30000u);
    assert(e.rx_queue[0].carrier_hz==38000u);
    assert(w.decoded_words==4 && w.invalid_events==0);
    assert(!lock_depth && locks==unlocks);
}
static void test_end_by_long_fifo_space(void) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,0);
    push_mark(&q,8500,0); push_space(&q,4200,0);
    /* Decoder outputs a 30-ms SPACE as a FIFO word rather than idle IRQ. */
    push_mark(&q,550,0); push_space(&q,30000,0);
    assert(gwir_amebad_rx_worker_step(&w,50u)==1);
    assert(e.rx_count==1 && e.rx_queue[0].count==3);
    assert(e.rx_queue[0].gap_after_us==30000u);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,50u)==0);
    assert(e.rx_count==1);
}
static void test_hw_error_recovery(void) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,0);
    push_mark(&q,9000,0);
    assert(gwir_amebad_rx_irq_error(&q)==0);
    push_space(&q,4500,0);push_mark(&q,560,0);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,50u)==0);
    assert(e.rx_count==0 && w.invalid_events==1);
    push_mark(&q,9000,0);push_space(&q,4500,0);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,50u)==1);
    assert(e.rx_count==1);
}
static void test_bad_counter_recovery(void) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,0);
    push_mark(&q,9000,0);
    assert(gwir_amebad_rx_irq_word(&q,0u)==0);
    push_space(&q,4500,0);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,20u)==0 && !e.rx_count);
    assert(w.invalid_events==1);
    push_mark(&q,9000,0);push_space(&q,4500,0);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,20u)==1 && e.rx_count==1);
}
static void test_overflow_discard_and_resync(void) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,0);
    for(unsigned i=0;i<GWIR_AMEBAD_RX_SW_DEPTH;i++)
        assert(gwir_amebad_rx_irq_word(&q,word_us(560u,!(i&1u)))==0);
    assert(q.used==GWIR_AMEBAD_RX_SW_DEPTH);
    assert(gwir_amebad_rx_irq_word(&q,word_us(560u,0))==-2);
    assert(q.overflows==1u && q.dropping);
    /* Full queue: resync needs space, so it is deferred. */
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==-2);
    assert(gwir_amebad_rx_worker_step(&w,1000u)==0);
    assert(e.rx_count==0 && w.discard_until_idle);
    assert(q.used==0u);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,50u)==0);
    assert(!w.discard_until_idle && !q.dropping);
    push_mark(&q,9000,0);push_space(&q,4500,0);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,50u)==1);
    assert(e.rx_count==1 && q.overflows>=1u);
}
static void test_vendor_irq_drain(void) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,0);
    hardware_size=4;hardware_read=0;
    hardware_fifo[0]=word_us(9000,0);
    hardware_fifo[1]=word_us(4500,1);
    hardware_fifo[2]=word_us(560,0);
    hardware_fifo[3]=word_us(560,1);
    assert(gwir_amebad_rx_vendor_irq_drain(&q)==4);
    assert(hardware_read==4);
    assert(gwir_amebad_rx_vendor_irq_drain(&q)==0);
    assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
    assert(gwir_amebad_rx_worker_step(&w,50u)==1);
    assert(e.rx_queue[0].count==4);
    hardware_size=0;hardware_read=0;
}
static void test_fifo_wrap_and_five_captures(void) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,0);
    for(unsigned j=0;j<300u;j++) {
        push_mark(&q,560u,0);
        push_space(&q,560u,0);
        assert(gwir_amebad_rx_irq_idle(&q,30000u)==0);
        assert(gwir_amebad_rx_worker_step(&w,3u)==1);
        assert(e.rx_count==1u);
        /* The test temporarily retires this acknowledged frame to exercise
           queue index wrap; actual app uses RX_ACK to retire captures. */
        e.rx_count=0u;
    }
    assert(q.used==0u && q.rd==q.wr);
    assert(q.rd != 0u); /* 900 entries wraps once around depth 512. */
    assert(w.decoded_words==600u);
}
static void test_vendor_full_32_fifo(void) {
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_queue_init(&q);
    hardware_size=32; hardware_read=0;
    for(uint32_t i=0;i<32u;i++) hardware_fifo[i]=word_us(560,i&1u);
    assert(gwir_amebad_rx_vendor_irq_drain(&q)==32);
    assert(q.used==32 && hardware_read==32);
    assert(gwir_amebad_rx_vendor_irq_drain(&q)==0);
    hardware_size=0;hardware_read=0;
}
static void test_invalid_init(void) {
    gwir_engine e;
    gwir_amebad_rx_queue q;
    gwir_amebad_rx_worker w;
    init(&e,&q,&w,0);
    gwir_amebad_rx_lock nolock={0};
    assert(gwir_amebad_rx_worker_init(&w,&q,&e,nolock,10000000u,0,38000u,333u,30000u)==-1);
    assert(gwir_amebad_rx_worker_init(&w,&q,&e,lock_ops(),0,0,38000u,333u,30000u)==-1);
    assert(gwir_amebad_rx_worker_init(&w,&q,&e,lock_ops(),10000000u,2,38000u,333u,30000u)==-1);
    assert(gwir_amebad_rx_irq_idle(&q,0u)==-1);
    assert(gwir_amebad_rx_worker_step(NULL,10u)==-1);
}
int main(void) {
    test_basic(0);test_basic(1);test_end_by_long_fifo_space();
    test_hw_error_recovery();test_bad_counter_recovery();
    test_overflow_discard_and_resync();test_vendor_irq_drain();test_invalid_init();
    test_fifo_wrap_and_five_captures();test_vendor_full_32_fifo();
    puts("PASS: AmebaD RX FIFO decode, polarity, idle, IRQ shim, overflow/error discard, resync");
    return 0;
}
