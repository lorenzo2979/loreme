#include "gwir_stream.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t encoded[GWIR_MAX_MESSAGE];
static size_t length;
static unsigned received;
static uint32_t sequences[16];

static int handler(void *context, const uint8_t *msg, size_t len) {
    (void)context;
    gwir_message parsed;
    assert(gwir_decode(msg,len,&parsed)==0);
    assert(received < sizeof(sequences)/sizeof(sequences[0]));
    sequences[received++] = parsed.sequence;
    return 0;
}
static int failing_handler(void *context,const uint8_t *msg,size_t len) {
    (void)context; (void)msg; (void)len;
    return 1;
}
int main(void) {
    static gwir_stream s;
    assert(gwir_encode(encoded,sizeof(encoded),GWIR_PING,123,NULL,0,&length)==0);
    assert(length==GWIR_HEADER_SIZE);
    gwir_stream_init(&s);
    /* Every split point of the 20-byte header and empty feeds. */
    for (size_t split=0;split<=length;++split) {
        gwir_stream_init(&s); received=0;
        assert(gwir_stream_feed(&s,encoded,split,handler,NULL)==(split==length ? 1 : 0));
        assert(gwir_stream_feed(&s,NULL,0,handler,NULL)==0);
        assert(gwir_stream_feed(&s,encoded+split,length-split,handler,NULL)==(split==length ? 0 : 1));
        assert(received==1 && sequences[0]==123 && s.filled==0);
    }
    /* Coalesced back-to-back messages and arbitrary one-byte chunks. */
    uint8_t many[GWIR_HEADER_SIZE*3];
    for (unsigned i=0;i<3;++i) {
        size_t n=0;
        assert(gwir_encode(many+i*GWIR_HEADER_SIZE,GWIR_HEADER_SIZE,GWIR_PING,i+1,NULL,0,&n)==0);
    }
    gwir_stream_init(&s); received=0;
    assert(gwir_stream_feed(&s,many,sizeof(many),handler,NULL)==3);
    assert(received==3 && sequences[0]==1 && sequences[1]==2 && sequences[2]==3);
    gwir_stream_init(&s); received=0;
    for (size_t i=0;i<sizeof(many);++i)
        assert(gwir_stream_feed(&s,many+i,1,handler,NULL)==(i%GWIR_HEADER_SIZE==GWIR_HEADER_SIZE-1 ? 1 : 0));
    assert(received==3);
    /* Maximum-size payload split at all sizes: header & data are validated. */
    static uint8_t maxmsg[GWIR_MAX_MESSAGE];
    static uint8_t payload[GWIR_MAX_PAYLOAD];
    for (size_t i=0;i<sizeof(payload);++i) payload[i]=(uint8_t)i;
    size_t maxlen=0;
    assert(gwir_encode(maxmsg,sizeof(maxmsg),GWIR_TX_FRAME,17,payload,sizeof(payload),&maxlen)==0);
    assert(maxlen==GWIR_MAX_MESSAGE);
    gwir_stream_init(&s);received=0;
    for (size_t offset=0;offset<maxlen;) {
        size_t chunk=(offset%37)+1;
        if (chunk>maxlen-offset) chunk=maxlen-offset;
        int got=gwir_stream_feed(&s,maxmsg+offset,chunk,handler,NULL);
        assert(got==(offset+chunk==maxlen ? 1 : 0));
        offset+=chunk;
    }
    assert(received==1 && sequences[0]==17 && s.messages_ok==1);
    /* Broken magic and oversized declared payload fail before buffer overflow. */
    uint8_t bad[GWIR_HEADER_SIZE];
    memcpy(bad,encoded,length); bad[0]='X';
    gwir_stream_init(&s);
    assert(gwir_stream_feed(&s,bad,length,handler,NULL)==-1 && s.failed);
    assert(gwir_stream_feed(&s,encoded,length,handler,NULL)==-1);
    memcpy(bad,encoded,length);
    gwir_write_u32be(bad+12,GWIR_MAX_PAYLOAD+1u);
    gwir_stream_init(&s);
    assert(gwir_stream_feed(&s,bad,length,handler,NULL)==-1 && s.messages_invalid==1);
    /* Corrupted checksum is rejected after full frame, never dispatched. */
    memcpy(bad,encoded,length);bad[19]^=0x80;
    gwir_stream_init(&s);received=0;
    assert(gwir_stream_feed(&s,bad,length,handler,NULL)==-1 && received==0);
    /* Application handler failure makes session unusable. */
    gwir_stream_init(&s);
    assert(gwir_stream_feed(&s,encoded,length,failing_handler,NULL)==-1);
    assert(gwir_stream_feed(&s,encoded,length,handler,NULL)==-1);
    /* Feeding a NULL pointer with positive byte length is rejected. */
    gwir_stream_init(&s);
    assert(gwir_stream_feed(&s,NULL,1,handler,NULL)==-1);
    printf("PASS stream fragmented/coalesced/header/max-frame/CRC/failure cases\n");
    return 0;
}
