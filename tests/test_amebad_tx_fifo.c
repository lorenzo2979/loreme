#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/realtek/gwir_amebad_tx_fifo.h"
#include "../firmware/realtek/gwir_amebad_tx_vendor.h"
#include "../firmware/realtek/gwir_amebad_tx_codec.h"
#include "../firmware/core/gwir_protocol.h"
#include "ameba_soc.h"

IR_TypeDef gwir_ir_test_device;

typedef struct {
    uint32_t fifo[32];
    uint32_t in_fifo;
    uint32_t collected[GWIR_MAX_PULSES_PER_FRAME];
    uint32_t collected_count;
    uint32_t sent;
    uint32_t end_count;
    uint32_t start_count;
    uint32_t batch_count;
    int running;
    int fail_write;
} fake;
static fake f;
static void reset(void) { memset(&f, 0, sizeof f); }
static uint32_t free_slots(void *ctx) { fake *p = ctx; return 32u - p->in_fifo; }
static int write_words(void *ctx, const uint32_t *words, uint32_t n, int last)
{
    fake *p = ctx;
    if (p->fail_write) return -1;
    assert(n != 0u && n <= 32u - p->in_fifo);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t w = words[i];
        assert((w & 0x40000000u) == 0u);
        if (last && i == n - 1u) { w |= 0x40000000u; p->end_count++; }
        p->fifo[p->in_fifo++] = w;
    }
    p->batch_count++;
    return 0;
}
static int start_tx(void *ctx) { fake *p = ctx; p->start_count++; p->running=1; return 0; }
static int is_running(void *ctx) { return ((fake *)ctx)->running; }
static void drain(fake *p, uint32_t count)
{
    assert(count <= p->in_fifo);
    for (uint32_t i=0; i<count; i++) {
        assert(p->collected_count < GWIR_MAX_PULSES_PER_FRAME);
        p->collected[p->collected_count++] = p->fifo[i];
    }
    memmove(p->fifo,p->fifo+count,(p->in_fifo-count)*sizeof(p->fifo[0]));
    p->in_fifo -= count;
    p->sent += count;
    if (!p->in_fifo && p->end_count && p->sent == p->collected_count) p->running=0;
}

/* The mock here simulates a controller, without measuring real IR timing. */
static void test_size(uint32_t count)
{
    uint32_t durations[GWIR_MAX_PULSES_PER_FRAME], words[GWIR_MAX_PULSES_PER_FRAME];
    for (uint32_t i=0;i<count;i++) durations[i]= (i%2u)?560u:900u;
    assert(gwir_amebad_encode_tx(durations,(uint16_t)count,38000u,words,2048u)==0);
    reset();
    gwir_amebad_tx_session s={0};
    assert(gwir_amebad_tx_begin(&s,words,count)==0);
    const gwir_amebad_tx_ops ops={&f,free_slots,write_words,start_tx,is_running};
    int ret=0; unsigned guard=0;
    while ((ret=gwir_amebad_tx_step(&s,&ops)) != GWIR_AMEBAD_TX_DONE) {
        assert(ret==GWIR_AMEBAD_TX_PROGRESS || ret==GWIR_AMEBAD_TX_WAIT);
        if (f.in_fifo) {
            uint32_t take=f.in_fifo>9u?9u:f.in_fifo;
            /* Do not let mock signal TX complete until last word queued. */
            drain(&f,take);
        }
        assert(++guard < 3000u);
    }
    assert(s.next==count && s.last_queued && s.completed && !s.failed);
    assert(f.end_count==1u && f.start_count==1u && f.sent==count);
    for(uint32_t i=0;i<count;i++) {
        const uint32_t expect=words[i] | ((i==count-1u)?0x40000000u:0u);
        assert(f.collected[i]==expect);
    }
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_DONE);
}
static void test_wait_error(void)
{
    const uint32_t words[]={0x80000026u,0x00000020u};
    gwir_amebad_tx_session s={0};
    reset();
    assert(gwir_amebad_tx_begin(NULL,words,2u)<0);
    assert(gwir_amebad_tx_begin(&s,words,2049u)<0);
    assert(gwir_amebad_tx_begin(&s,words,2u)==0);
    gwir_amebad_tx_ops ops={&f,free_slots,write_words,start_tx,is_running};
    f.in_fifo=32u;
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_WAIT);
    assert(!s.started && !s.next);
    f.in_fifo=0;
    f.fail_write=1;
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_ERROR);
    assert(s.failed && !s.next && !s.started);
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_ERROR);
    reset();
    assert(gwir_amebad_tx_begin(&s,words,2u)==0);
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_PROGRESS);
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_WAIT);
    assert(!s.completed);
    /* Controller FIFO empty while FSM busy is not completion. */
    drain(&f,2u);
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_DONE);
}

/* Check wrapper against SDK-like function prototypes, not target hardware. */
u32 IR_GetTxFIFOFreeLen(IR_TypeDef *p){assert(p==IR_DEV);return free_slots(&f);}
void IR_SendBuf(IR_TypeDef *p, u32 *words, u32 n, u32 last){assert(p==IR_DEV);assert(write_words(&f,words,n,last)!=-1);}
void IR_Cmd(IR_TypeDef *p, u32 mode, u32 state){assert(p==IR_DEV && mode==IR_MODE_TX && state==ENABLE);assert(start_tx(&f)==0);}
u32 IR_FSMRunning(IR_TypeDef *p){assert(p==IR_DEV);return (u32)is_running(&f);}
static void test_vendor_adapter(void)
{
    const uint32_t words[]={0x80000028u,0x00000028u};
    reset();
    gwir_amebad_tx_session s={0};
    assert(gwir_amebad_tx_begin(&s,words,2u)==0);
    const gwir_amebad_tx_ops ops=gwir_amebad_vendor_tx_ops();
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_PROGRESS);
    drain(&f,2u);
    assert(gwir_amebad_tx_step(&s,&ops)==GWIR_AMEBAD_TX_DONE);
    assert(f.start_count==1u && f.end_count==1u);
}
int main(void)
{
    test_size(1u); test_size(2u); test_size(31u);
    test_size(32u); test_size(33u); test_size(64u);
    test_size(2048u); test_wait_error(); test_vendor_adapter();
    puts("PASS: AmebaD TX FIFO 1..2048 words, refill, final flag, FSM, SDK shim");
    return 0;
}
