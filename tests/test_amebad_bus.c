#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/realtek/gwir_amebad_bus.h"

typedef struct {
    int authorized, rx, tx, fail_stop_tx, fail_prep;
    int starts_rx, stops_rx, prep, stops_tx, starts_tx;
    uint32_t ms, occupancy, queued, next_limit, sent[16], markers;
    unsigned count;
} fake;
static int ready(void *p){return ((fake*)p)->authorized;}
static int start_rx(void *p){fake *f=p; ++f->starts_rx; f->rx=1;return 0;}
static int stop_rx(void *p){fake *f=p; ++f->stops_rx;f->rx=0;return 0;}
static int prepare(void *p,uint32_t hz,uint16_t duty){fake *f=p;++f->prep;
    assert(!f->rx && !f->tx && hz==38000u && duty==333u);
    if(f->fail_prep) return -1;
    f->tx=1;return 0;}
static int stop_tx(void *p){fake *f=p;++f->stops_tx;if(f->fail_stop_tx)return -1;f->tx=0;return 0;}
static uint32_t now(void *p){return ((fake*)p)->ms;}
static uint32_t free_slots(void *p){fake *f=p;return (f->next_limit ? f->next_limit : 32u)-f->occupancy;}
static int write_words(void *p,const uint32_t *v,uint32_t n,int last){fake *f=p;
    assert(f->tx);assert(n<=free_slots(p));assert(f->count+n<=16);
    for(uint32_t i=0;i<n;i++)f->sent[f->count++]=v[i];
    f->occupancy+=n;f->queued+=n;if(last){++f->markers;}return 0;}
static int start_tx(void *p){fake *f=p; ++f->starts_tx;return 0;}
static int running(void *p){return ((fake*)p)->occupancy?1:0;}
static gwir_amebad_bus_ops cfg(fake *f){gwir_amebad_bus_ops o={0};o.ctx=f;
    o.board_ir_ready=ready;o.rx_start=start_rx;o.rx_stop=stop_rx;
    o.tx_prepare=prepare;o.tx_stop=stop_tx;o.monotonic_ms=now;
    o.tx.ctx=f;o.tx.free_slots=free_slots;o.tx.write_words=write_words;
    o.tx.start_tx=start_tx;o.tx.is_running=running;return o;}
static void test_gating_and_rx(void){fake f={0};gwir_amebad_bus b;
    gwir_amebad_bus_ops o=cfg(&f);
    assert(gwir_amebad_bus_init(&b,&o)==0);
    f.next_limit=2;
    assert(gwir_amebad_bus_start_rx(&b)==GWIR_BUS_REJECTED);
    assert(f.starts_rx==0 && f.prep==0);
    f.authorized=1;
    assert(gwir_amebad_bus_start_rx(&b)==0 && b.state==GWIR_BUS_RX);
    const uint32_t d[]={9000u,4500u};
    assert(gwir_amebad_bus_start_tx(&b,d,2,38000,333,1000,10000)==GWIR_BUS_REJECTED);
    assert(f.prep==0);
    assert(gwir_amebad_bus_stop_rx(&b)==0 && b.state==GWIR_BUS_OFF);
    assert(f.stops_rx==1);
    assert(gwir_amebad_bus_start_tx(&b,d,2,38000,333,1000,10000)==0);
    assert(f.prep==1 && b.state==GWIR_BUS_TX);
    assert(gwir_amebad_bus_start_rx(&b)==GWIR_BUS_REJECTED);
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_PROGRESS);
    assert(f.count==2 && f.starts_tx==1 && f.markers==1);
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_WAIT);
    f.occupancy=0;f.next_limit=32;
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_WAIT);
    assert(b.state==GWIR_BUS_GAP && f.stops_tx==1);
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_WAIT);
    ++f.ms;
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_FINISHED);
    assert(b.tx_completed==1 && b.state==GWIR_BUS_OFF);
    assert(gwir_amebad_bus_start_rx(&b)==0);
    assert(gwir_amebad_bus_stop_rx(&b)==0);
}
static void test_chunking_timeout_and_wrap(void){fake f={0};gwir_amebad_bus b;
    gwir_amebad_bus_ops o=cfg(&f);f.authorized=1;
    assert(gwir_amebad_bus_init(&b,&o)==0);
    f.next_limit=2;
    uint32_t d[]={560,560,1690,560,560};
    f.ms=UINT32_MAX-2u;
    assert(gwir_amebad_bus_start_tx(&b,d,5,38000,333,0,5000)==0);
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_PROGRESS && f.count==2);
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_WAIT);
    f.occupancy=0; f.ms=UINT32_MAX;
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_PROGRESS && f.count==4);
    f.occupancy=0;f.ms=1;
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_PROGRESS && f.count==5);
    assert(f.markers==1 && f.starts_tx==1);
    f.occupancy=0;f.next_limit=32;
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_FINISHED);
    assert(f.stops_tx==1 && b.tx_completed==1);
    assert(f.sent[0]&GWIR_AMEBAD_IR_CARRIER_FLAG);
    assert(!(f.sent[1]&GWIR_AMEBAD_IR_CARRIER_FLAG));
}
static void test_refusal_and_faults(void){fake f={0};gwir_amebad_bus b;
    gwir_amebad_bus_ops o=cfg(&f);f.authorized=1;
    assert(gwir_amebad_bus_init(&b,&o)==0);
    uint32_t bad[]={0,560},ok[]={560,560};
    assert(gwir_amebad_bus_start_tx(&b,bad,2,38000,333,0,1000)==GWIR_BUS_REJECTED);
    assert(f.prep==0);
    assert(gwir_amebad_bus_start_tx(&b,ok,2,38000,333,0,1)==GWIR_BUS_REJECTED);
    assert(f.prep==0);
    f.fail_prep=1;
    assert(gwir_amebad_bus_start_tx(&b,ok,2,38000,333,0,100)==GWIR_BUS_ERROR);
    assert(f.stops_tx==1 && b.state==GWIR_BUS_OFF);
    f.fail_prep=0; f.ms=42;
    assert(gwir_amebad_bus_start_tx(&b,ok,2,38000,333,0,100)==0);
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_PROGRESS);
    f.ms=143;
    assert(gwir_amebad_bus_poll(&b)==GWIR_BUS_ERROR);
    assert(b.state==GWIR_BUS_OFF && f.stops_tx==2);
    assert(b.errors==2);
    assert(gwir_amebad_bus_start_tx(&b,ok,2,38000,333,0,100)==0);
    f.fail_stop_tx=1;
    assert(gwir_amebad_bus_abort(&b)==GWIR_BUS_ERROR && b.state==GWIR_BUS_FAULT);
    assert(gwir_amebad_bus_start_rx(&b)==GWIR_BUS_REJECTED);
    f.fail_stop_tx=0;
    assert(gwir_amebad_bus_abort(&b)==0 && b.state==GWIR_BUS_OFF);
    f.authorized=0;
    assert(gwir_amebad_bus_start_tx(&b,ok,2,38000,333,0,100)==GWIR_BUS_REJECTED);
    assert(f.prep==3);
}
int main(void){test_gating_and_rx();test_chunking_timeout_and_wrap();test_refusal_and_faults();
    puts("PASS: half duplex board gate, RX/TX exclusion, chunked TX, timeout, wrap, gap, fault recovery");return 0;}
