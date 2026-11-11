#include "gwir_engine.h"
#include "gwir_capture_builder.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tx_calls=0,arm_calls=0,off_calls=0;
static uint32_t sent_gap=0;
static int fake_tx(void *ctx, uint32_t hz, uint16_t duty, const uint32_t *d, uint16_t n, uint32_t gap) {
    (void)ctx;
    assert(hz==38000 && duty==333 && n==4 && d[0]==9000 && d[1]==4500 && d[2]==560 && d[3]==560);
    tx_calls++; sent_gap=gap; return 0;
}
static int fake_arm(void *ctx) {(void)ctx;arm_calls++;return 0;}
static int fake_off(void *ctx) {(void)ctx;off_calls++;return 0;}
static unsigned char req[GWIR_MAX_MESSAGE], resp[GWIR_MAX_MESSAGE];
static size_t reqsz, respsz;
static gwir_message decoded;
static gwir_engine engine;

static void transact(uint8_t cmd,const uint8_t *payload,uint32_t length,uint32_t sequence) {
    assert(gwir_encode(req,sizeof(req),cmd,sequence,payload,length,&reqsz)==0);
    assert(gwir_engine_process(&engine,req,reqsz,resp,sizeof(resp),&respsz)==0);
    assert(gwir_decode(resp,respsz,&decoded)==0);
    assert(decoded.sequence==sequence);
}
static uint32_t current_capture_id(void) {
    assert(decoded.command==GWIR_RX_DATA && decoded.payload_len>=GWIR_RX_FIXED_BYTES);
    uint32_t id=gwir_read_u32be(decoded.payload);
    assert(id!=0);
    return id;
}
static void ack_capture(uint32_t ack_seq) {
    uint8_t ack[4];
    gwir_write_u32be(ack,current_capture_id());
    transact(GWIR_RX_ACK,ack,sizeof(ack),ack_seq);
    assert(decoded.command==GWIR_STATUS && decoded.payload_len==1 && decoded.payload[0]==GWIR_OK);
}
static void status_is(uint8_t s) { assert(decoded.command==GWIR_STATUS && decoded.payload_len==1 && decoded.payload[0]==s); }
static void test_protocol(void) {
    unsigned char bytes[24]; size_t len=0; gwir_message v;
    assert(gwir_encode(bytes,sizeof(bytes),GWIR_PING,0x89ABCDEFu,NULL,0,&len)==0 && len==20);
    assert(gwir_decode(bytes,len,&v)==0 && v.sequence==0x89ABCDEFu);
    assert(gwir_decode(bytes,len-1,&v)!=0);
    bytes[11]^=1;assert(gwir_decode(bytes,len,&v)!=0);bytes[11]^=1;
    bytes[4]=2;assert(gwir_decode(bytes,len,&v)!=0);bytes[4]=3;
    bytes[6]=1;assert(gwir_decode(bytes,len,&v)!=0);bytes[6]=0;
    bytes[12]=255;assert(gwir_decode(bytes,len,&v)!=0);bytes[12]=0;
    assert(gwir_encode(bytes,3,GWIR_PING,1,NULL,0,&len)!=0);
    assert(gwir_encode(bytes,sizeof(bytes),GWIR_PING,1,NULL,1,&len)!=0);
    assert(gwir_encode(bytes,sizeof(bytes),GWIR_PING,1,NULL,GWIR_MAX_PAYLOAD+1,&len)!=0);
    assert(gwir_crc32_continue(0xFFFFFFFFu,(const uint8_t *)"123456789",9)^0xFFFFFFFFu ? 1:0);
    assert((gwir_crc32_continue(0xFFFFFFFFu,(const uint8_t *)"123456789",9)^0xFFFFFFFFu)==0xCBF43926u);
}
int main(void) {
    test_protocol();
    gwir_ir_hal h={fake_tx,fake_arm,fake_off,NULL}; gwir_engine_init(&engine,h);
    transact(GWIR_HELLO,NULL,0,1);
    assert(decoded.command==GWIR_HELLO_REPLY && decoded.payload_len==8);
    assert(decoded.payload[0]==3 && decoded.payload[1]==5 && gwir_read_u16be(decoded.payload+2)==2048);
    assert(decoded.payload[4]&4u); /* ACK-based delivery */
    transact(GWIR_PING,NULL,0,2);assert(decoded.command==GWIR_PONG);
    transact(GWIR_RX_READ,NULL,0,3);status_is(GWIR_NO_CAPTURE);
    uint32_t capture[4]={9000,4500,560,560};
    assert(gwir_engine_capture(&engine,38000,333,capture,4,40000)!=0);
    transact(GWIR_RX_ARM,NULL,0,4);status_is(GWIR_OK); assert(arm_calls==1);
    assert(gwir_engine_capture(&engine,38000,333,capture,4,40000)==0);
    transact(GWIR_RX_STATS,NULL,0,40); assert(decoded.command==GWIR_RX_STATS_REPLY && decoded.payload[1]==1);
    transact(GWIR_RX_READ,NULL,0,5); assert(decoded.command==GWIR_RX_DATA && decoded.payload_len==32);
    uint32_t initial_id=current_capture_id();
    assert(gwir_read_u32be(decoded.payload+4)==38000 && gwir_read_u16be(decoded.payload+10)==4);
    assert(gwir_read_u32be(decoded.payload+16)==9000);
    /* READ does not delete data. Same capture ID survives a dropped response. */
    transact(GWIR_RX_READ,NULL,0,501);
    assert(current_capture_id()==initial_id);
    uint8_t wrong_ack[4]; gwir_write_u32be(wrong_ack,initial_id+123u);
    transact(GWIR_RX_ACK,wrong_ack,4,502);status_is(GWIR_NO_CAPTURE);
    transact(GWIR_RX_STATS,NULL,0,503);assert(decoded.payload[1]==1);
    transact(GWIR_RX_READ,NULL,0,504);ack_capture(505);
    /* Duplicate ACK after a lost ACK reply must be harmless. */
    uint8_t repeated_ack[4];gwir_write_u32be(repeated_ack,initial_id);
    transact(GWIR_RX_ACK,repeated_ack,4,506);status_is(GWIR_OK);
    transact(GWIR_RX_READ,NULL,0,6);status_is(GWIR_NO_CAPTURE);
    unsigned char body[28];
    gwir_write_u32be(body,38000);gwir_write_u16be(body+4,333);
    gwir_write_u16be(body+6,4);gwir_write_u32be(body+8,60000);
    for(int i=0;i<4;i++)gwir_write_u32be(body+12+4*i,capture[i]);
    transact(GWIR_TX_FRAME,body,sizeof(body),70);status_is(GWIR_BUSY);
    transact(GWIR_RX_DISARM,NULL,0,71);status_is(GWIR_OK);
    transact(GWIR_TX_FRAME,body,sizeof(body),7);status_is(GWIR_OK);
    assert(tx_calls==1 && sent_gap==60000);
    gwir_write_u32be(body,12000);transact(GWIR_TX_FRAME,body,sizeof(body),8);status_is(GWIR_RANGE);
    gwir_write_u32be(body,38000);gwir_write_u16be(body+6,2049);
    transact(GWIR_TX_FRAME,body,sizeof(body),9);status_is(GWIR_RANGE);
    gwir_write_u16be(body+6,4);
    transact(GWIR_TX_FRAME,body,sizeof(body)-4,10);status_is(GWIR_RANGE);
    transact(GWIR_RX_DISARM,NULL,0,11);status_is(GWIR_OK);assert(off_calls==1);
    transact(0x77,NULL,0,12);status_is(GWIR_UNSUPPORTED);
    /* Five ordered frames captured rapidly; Android may read them later. */
    transact(GWIR_RX_ARM,NULL,0,100);status_is(GWIR_OK);
    for (int i=0;i<5;i++) {
        uint32_t pulses[2]={(uint32_t)(1000+i),2000};
        assert(gwir_engine_capture(&engine,38000,333,pulses,2,(uint32_t)(5000+i))==0);
    }
    assert(gwir_engine_capture(&engine,38000,333,capture,4,5000)==-2);
    transact(GWIR_RX_STATS,NULL,0,101);
    assert(decoded.command==GWIR_RX_STATS_REPLY);
    assert(decoded.payload[0]==1 && decoded.payload[1]==5 && decoded.payload[2]==5);
    assert(gwir_read_u16be(decoded.payload+4)==1);
    /* Stop IR hardware but retain all queued frames. */
    transact(GWIR_RX_DISARM,NULL,0,102);status_is(GWIR_OK);
    for (int i=0;i<5;i++) {
        transact(GWIR_RX_READ,NULL,0,(uint32_t)(103+i));
        assert(decoded.command==GWIR_RX_DATA);
        assert(gwir_read_u32be(decoded.payload+16)==(uint32_t)(1000+i));
        assert(gwir_read_u32be(decoded.payload+12)==(uint32_t)(5000+i));
        if(i==0) { /* no re-arming while unread frames remain */
            transact(GWIR_RX_ARM,NULL,0,400);status_is(GWIR_BUSY);
            transact(GWIR_RX_READ,NULL,0,401);
        }
        ack_capture((uint32_t)(410+i));
    }
    transact(GWIR_RX_READ,NULL,0,110);status_is(GWIR_NO_CAPTURE);
    transact(GWIR_RX_STATS,NULL,0,111);
    assert(decoded.payload[1]==0 && gwir_read_u16be(decoded.payload+4)==1);
    /* RX_ARM resets the overrun counter and queue. */
    transact(GWIR_RX_ARM,NULL,0,112);status_is(GWIR_OK);
    transact(GWIR_RX_STATS,NULL,0,113);
    assert(decoded.payload[1]==0 && gwir_read_u16be(decoded.payload+4)==0);
    transact(GWIR_RX_DISARM,NULL,0,114);status_is(GWIR_OK);
    /* A full 2048-entry frame survives serialization. */
    static uint32_t full[GWIR_MAX_PULSES_PER_FRAME];
    for (unsigned i=0;i<GWIR_MAX_PULSES_PER_FRAME;i++) full[i]=500+i;
    transact(GWIR_RX_ARM,NULL,0,120);status_is(GWIR_OK);
    assert(gwir_engine_capture(&engine,38000,333,full,GWIR_MAX_PULSES_PER_FRAME,40000)==0);
    transact(GWIR_RX_DISARM,NULL,0,121);status_is(GWIR_OK);
    transact(GWIR_RX_READ,NULL,0,122);
    assert(decoded.command==GWIR_RX_DATA && decoded.payload_len==GWIR_MAX_PAYLOAD);
    for (unsigned i=0;i<GWIR_MAX_PULSES_PER_FRAME;i++) {
        assert(gwir_read_u32be(decoded.payload+16+i*4)==500+i);
    }
    ack_capture(421);
    /* A demodulating receiver produces levels/timings, not full decoded protocols. */
    static gwir_capture_builder b;
    gwir_capture_builder_init(&b,&engine,38000,333,20000);
    transact(GWIR_RX_ARM,NULL,0,130);status_is(GWIR_OK);
    assert(gwir_capture_builder_feed(&b,0,5000)==0); /* ignore leading idle */
    for (int i=0;i<5;i++) {
        assert(gwir_capture_builder_feed(&b,1,(uint32_t)(9000+i))==0);
        assert(gwir_capture_builder_feed(&b,0,4500)==0);
        assert(gwir_capture_builder_feed(&b,1,560)==0);
        assert(gwir_capture_builder_feed(&b,0,(uint32_t)(25000+i))==1);
    }
    transact(GWIR_RX_DISARM,NULL,0,131);status_is(GWIR_OK);
    transact(GWIR_RX_STATS,NULL,0,132);
    assert(decoded.payload[1]==5 && gwir_read_u16be(decoded.payload+4)==0);
    for (int i=0;i<5;i++) {
        transact(GWIR_RX_READ,NULL,0,(uint32_t)(140+i));
        assert(decoded.command==GWIR_RX_DATA);
        assert(gwir_read_u16be(decoded.payload+10)==3);
        assert(gwir_read_u32be(decoded.payload+16)==(uint32_t)(9000+i));
        assert(gwir_read_u32be(decoded.payload+12)==(uint32_t)(25000+i));
        ack_capture((uint32_t)(450+i));
    }
    assert(b.completed==5 && b.rejected==0);
    /* Merge repeated MARK levels (e.g. in an interrupt's split batch). */
    transact(GWIR_RX_ARM,NULL,0,150);status_is(GWIR_OK);
    assert(gwir_capture_builder_feed(&b,1,9000)==0);
    assert(gwir_capture_builder_feed(&b,1,100)==0);
    assert(gwir_capture_builder_feed(&b,0,30000)==1);
    transact(GWIR_RX_DISARM,NULL,0,151);status_is(GWIR_OK);
    transact(GWIR_RX_READ,NULL,0,152);
    assert(decoded.command==GWIR_RX_DATA && gwir_read_u32be(decoded.payload+16)==9100);
    ack_capture(460);
    /* Last frame: receiver may only emit a timeout, not a new SPACE edge. */
    transact(GWIR_RX_ARM,NULL,0,160);status_is(GWIR_OK);
    assert(gwir_capture_builder_feed(&b,1,9000)==0);
    assert(gwir_capture_builder_feed(&b,0,4500)==0);
    assert(gwir_capture_builder_feed(&b,1,560)==0);
    assert(gwir_capture_builder_finish_idle(&b,19999)==-1); /* not idle enough */
    assert(gwir_capture_builder_finish_idle(&b,25000)==1);
    assert(gwir_capture_builder_finish_idle(&b,25000)==0);
    transact(GWIR_RX_DISARM,NULL,0,161);status_is(GWIR_OK);
    transact(GWIR_RX_READ,NULL,0,162);
    assert(decoded.command==GWIR_RX_DATA);
    assert(gwir_read_u16be(decoded.payload+10)==3);
    assert(gwir_read_u32be(decoded.payload+12)==25000);
    ack_capture(470);
    printf("PASS CRC framing, acknowledged five-frame FIFO, RAW timing assembler, idle flush, overflow, 2048 timings, bounds\n");
    return 0;
}
