#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ameba_soc.h"
#include "../firmware/core/gwir_engine.h"
#include "../firmware/realtek/gwir_amebad_rx_irq.h"
IR_TypeDef gwir_ir_test_device;
static u32 irq_flags, masked, cleared, mask_off, mask_on;
static u32 fifo[32], n_fifo, read_pos, clear_fifo_count;
static unsigned lock_depth;
static void lock_irq(void *ctx) {(void)ctx;assert(lock_depth==0);lock_depth=1;}
static void unlock_irq(void *ctx) {(void)ctx;assert(lock_depth==1);lock_depth=0;}
u32 IR_GetINTStatus(IR_TypeDef *ir) {assert(ir==IR_DEV);return irq_flags;}
u32 IR_GetIMR(IR_TypeDef *ir) {assert(ir==IR_DEV);return masked;}
void IR_MaskINTConfig(IR_TypeDef *ir,u32 m,u32 on) {
    assert(ir==IR_DEV);
    if(on==DISABLE){assert(m==IR_RX_INT_ALL_MASK);++mask_off;}
    else {assert(on==ENABLE && m==masked);++mask_on;}
}
void IR_ClearINTPendingBit(IR_TypeDef *ir,u32 m) {assert(ir==IR_DEV);cleared|=m;}
void IR_ClearRxFIFO(IR_TypeDef *ir) {assert(ir==IR_DEV);read_pos=n_fifo;++clear_fifo_count;}
u32 IR_GetRxDataLen(IR_TypeDef *ir) {assert(ir==IR_DEV);return n_fifo-read_pos;}
u32 IR_ReceiveData(IR_TypeDef *ir) {assert(ir==IR_DEV && read_pos<n_fifo);return fifo[read_pos++];}
static u32 rx_us(u32 us,int high) {return us*10u | (high?0x80000000u:0u);}
static void setup(gwir_engine *e,gwir_amebad_rx_queue *q,gwir_amebad_rx_worker *w) {
    gwir_ir_hal h={0};gwir_engine_init(e,h);e->rx_armed=1;
    gwir_amebad_rx_queue_init(q);
    gwir_amebad_rx_lock l={lock_irq,unlock_irq,NULL};
    assert(gwir_amebad_rx_worker_init(w,q,e,l,10000000u,0u,38000u,333u,30000u)==0);
    irq_flags=cleared=mask_off=mask_on=n_fifo=read_pos=clear_fifo_count=0;
    masked=IR_RX_INT_ALL_MASK;
}
static void test_idle_drain(void) {
    gwir_engine e;gwir_amebad_rx_queue q;gwir_amebad_rx_worker w;
    gwir_amebad_rx_irq_stats s={0};setup(&e,&q,&w);
    fifo[0]=rx_us(9000,0);fifo[1]=rx_us(4500,1);
    fifo[2]=rx_us(560,0);fifo[3]=rx_us(560,1);n_fifo=4;
    irq_flags=IR_RX_CNT_THR_INT_STATUS;
    assert(gwir_amebad_rx_irq_service(&q,30000u,&s)==4);
    assert(read_pos==4 && q.used==5);
    assert(cleared==IR_RX_CNT_THR_INT_CLR && mask_off==1 && mask_on==1);
    assert(s.invocations==1 && s.fifo_words==4 && s.idle_events==1 && s.hardware_errors==0);
    assert(gwir_amebad_rx_worker_step(&w,20)==1);
    assert(e.rx_count==1 && e.rx_queue[0].count==4);
}
static void test_hw_overflow_discard(void) {
    gwir_engine e;gwir_amebad_rx_queue q;gwir_amebad_rx_worker w;
    gwir_amebad_rx_irq_stats s={0};setup(&e,&q,&w);
    fifo[0]=rx_us(9000,0);n_fifo=1;
    irq_flags=IR_RX_FIFO_OF_INT_STATUS | IR_RX_CNT_THR_INT_STATUS;
    assert(gwir_amebad_rx_irq_service(&q,30000u,&s)==-2);
    assert(read_pos==n_fifo && clear_fifo_count==1);
    assert(q.used==2 && s.hardware_errors==1 && s.idle_events==1);
    assert(gwir_amebad_rx_worker_step(&w,10)==0 && e.rx_count==0);
    assert(w.invalid_events==1 && !w.discard_until_idle);
    fifo[0]=rx_us(9000,0);fifo[1]=rx_us(4500,1);n_fifo=2;read_pos=0;
    irq_flags=IR_RX_FIFO_LEVEL_INT_STATUS | IR_RX_CNT_THR_INT_STATUS;
    assert(gwir_amebad_rx_irq_service(&q,30000u,&s)==2);
    assert(gwir_amebad_rx_worker_step(&w,10)==1 && e.rx_count==1);
}
static void test_bind_unbind_and_bad_args(void) {
    gwir_amebad_rx_queue q,other;gwir_amebad_rx_irq_stats stats={0};
    gwir_amebad_rx_queue_init(&q);gwir_amebad_rx_queue_init(&other);
    assert(gwir_amebad_rx_irq_service(NULL,30000u,&stats)==-1);
    assert(gwir_amebad_rx_irq_service(&q,0u,&stats)==-1);
    assert(gwir_amebad_rx_irq_bind(NULL,30000u,&stats)==-1);
    assert(gwir_amebad_rx_irq_bind(&q,30000u,&stats)==0);
    assert(gwir_amebad_rx_irq_bind(&other,30000u,&stats)==-2);
    irq_flags=0;gwir_amebad_rx_irq_entry();assert(q.used==0);
    gwir_amebad_rx_irq_unbind();
    irq_flags=IR_RX_CNT_THR_INT_STATUS;gwir_amebad_rx_irq_entry();assert(q.used==0);
}
static void test_ring_overflow_is_rejected(void) {
    gwir_amebad_rx_queue q;gwir_amebad_rx_queue_init(&q);
    for(unsigned i=0;i<GWIR_AMEBAD_RX_SW_DEPTH;i++)
        assert(gwir_amebad_rx_irq_word(&q,rx_us(500,0))==0);
    n_fifo=1;read_pos=0;fifo[0]=rx_us(560,0);
    irq_flags=IR_RX_FIFO_LEVEL_INT_STATUS;
    assert(gwir_amebad_rx_irq_service(&q,30000u,NULL)==-2);
    assert(q.overflows==1u && q.dropping==1u);
    assert(q.used==GWIR_AMEBAD_RX_SW_DEPTH);
}
int main(void) {
    test_idle_drain();test_hw_overflow_discard();
    test_bind_unbind_and_bad_args();test_ring_overflow_is_rejected();
    puts("PASS: SDK-shaped RX IRQ: timeout tail drain, error discard, resync, binding, queue overflow");
    return 0;
}
