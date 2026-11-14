#include "gwir_engine.h"
#include <limits.h>
#include <string.h>

static int values_valid(uint32_t carrier, uint16_t duty, const uint32_t *d, uint16_t n, uint32_t gap) {
    if (carrier < 25000 || carrier > 500000 || duty < 200 || duty > 500 ||
        n == 0 || n > GWIR_MAX_PULSES_PER_FRAME || gap > 30000000u || !d) return 0;
    for (uint16_t i=0; i<n; ++i) if (d[i] == 0 || d[i] > 30000000u) return 0;
    return 1;
}
void gwir_engine_init(gwir_engine *e, gwir_ir_hal hal) {
    if (!e) return;
    memset(e, 0, sizeof(*e));
    e->hal=hal;
}
int gwir_engine_capture(gwir_engine *e, uint32_t carrier, uint16_t duty,
                        const uint32_t *d, uint16_t n, uint32_t gap) {
    if (!e || !e->rx_armed || !values_valid(carrier,duty,d,n,gap)) return -1;
    if (e->rx_count == GWIR_MAX_FRAMES_PER_BUTTON) {
        if (e->rx_dropped != UINT16_MAX) ++e->rx_dropped;
        return -2; /* overflow; NEVER silently discard an earlier frame */
    }
    unsigned slot=((unsigned)e->rx_head+e->rx_count)%GWIR_MAX_FRAMES_PER_BUTTON;
    gwir_rx_frame *f=&e->rx_queue[slot];
    memcpy(f->durations_us,d,(size_t)n*sizeof(uint32_t));
    if (++e->next_capture_id == 0) ++e->next_capture_id;
    f->capture_id=e->next_capture_id;
    f->count=n;
    f->carrier_hz=carrier;
    f->duty_permille=duty;
    f->gap_after_us=gap;
    ++e->rx_count;
    return 0;
}
static int status_reply(uint8_t *response, size_t cap, uint32_t seq, uint8_t status, size_t *size) {
    return gwir_encode(response,cap,GWIR_STATUS,seq,&status,1,size);
}
int gwir_engine_process(gwir_engine *e, const uint8_t *request, size_t request_size,
                        uint8_t *response, size_t cap, size_t *response_size) {
    gwir_message m;
    if (!e || !response || !response_size || gwir_decode(request,request_size,&m)) return -1;
    uint8_t status = GWIR_OK;
    switch (m.command) {
    case GWIR_HELLO: {
        if (m.payload_len) return status_reply(response,cap,m.sequence,GWIR_BAD_REQUEST,response_size);
        uint8_t caps[8] = {GWIR_VERSION,GWIR_MAX_FRAMES_PER_BUTTON,0,0,0,0,0,0};
        gwir_write_u16be(caps+2,GWIR_MAX_PULSES_PER_FRAME);
        /* bit0: RX queue; bit1: carrier/duty are assumed; bit2: RX_ACK required */
        caps[4]=7;
        return gwir_encode(response,cap,GWIR_HELLO_REPLY,m.sequence,caps,sizeof(caps),response_size);
    }
    case GWIR_PING:
        if (m.payload_len) return status_reply(response,cap,m.sequence,GWIR_BAD_REQUEST,response_size);
        return gwir_encode(response,cap,GWIR_PONG,m.sequence,NULL,0,response_size);
    case GWIR_RX_ARM:
        if (m.payload_len) status=GWIR_BAD_REQUEST;
        else if (e->rx_armed || e->rx_count) status=GWIR_BUSY; /* unread data must be ACKed first */
        else if (!e->hal.rx_arm) status=GWIR_UNSUPPORTED;
        else if (e->hal.rx_arm(e->hal.ctx)) status=GWIR_IR_FAILED;
        else { e->rx_armed=1; e->rx_head=0; e->rx_count=0; e->rx_dropped=0; }
        return status_reply(response,cap,m.sequence,status,response_size);
    case GWIR_RX_DISARM:
        if (m.payload_len) status=GWIR_BAD_REQUEST;
        else if (!e->rx_armed) status=GWIR_OK;
        else if (!e->hal.rx_disarm) status=GWIR_UNSUPPORTED;
        else if (e->hal.rx_disarm(e->hal.ctx)) status=GWIR_IR_FAILED;
        else e->rx_armed=0; /* preserve queue until Android drains it */
        return status_reply(response,cap,m.sequence,status,response_size);
    case GWIR_RX_STATS: {
        if (m.payload_len) return status_reply(response,cap,m.sequence,GWIR_BAD_REQUEST,response_size);
        uint8_t stats[8]={e->rx_armed,e->rx_count,GWIR_MAX_FRAMES_PER_BUTTON,(uint8_t)(e->rx_dropped>0),0,0,0,0};
        gwir_write_u16be(stats+4,e->rx_dropped);
        return gwir_encode(response,cap,GWIR_RX_STATS_REPLY,m.sequence,stats,sizeof(stats),response_size);
    }
    case GWIR_TX_FRAME: {
        if (m.payload_len < GWIR_FRAME_FIXED_BYTES) status=GWIR_BAD_REQUEST;
        else if (e->rx_armed) status=GWIR_BUSY; /* one IR peripheral: no concurrent TX & RX */
        else {
            uint32_t carrier=gwir_read_u32be(m.payload);
            uint16_t duty=gwir_read_u16be(m.payload+4);
            uint16_t n=gwir_read_u16be(m.payload+6);
            uint32_t gap=gwir_read_u32be(m.payload+8);
            if (!n || n > GWIR_MAX_PULSES_PER_FRAME ||
                m.payload_len != GWIR_FRAME_FIXED_BYTES+(uint32_t)n*4u) status=GWIR_RANGE;
            else {
                for (uint16_t i=0; i<n; ++i) e->tx_durations[i] = gwir_read_u32be(m.payload+12+(size_t)i*4);
                if (!values_valid(carrier,duty,e->tx_durations,n,gap)) status=GWIR_RANGE;
                else if (!e->hal.tx) status=GWIR_UNSUPPORTED;
                else if (e->hal.tx(e->hal.ctx,carrier,duty,e->tx_durations,n,gap)) status=GWIR_IR_FAILED;
            }
        }
        return status_reply(response,cap,m.sequence,status,response_size);
    }
    case GWIR_RX_READ:
        if (m.payload_len) return status_reply(response,cap,m.sequence,GWIR_BAD_REQUEST,response_size);
        if (!e->rx_count) return status_reply(response,cap,m.sequence,GWIR_NO_CAPTURE,response_size);
        {
            const gwir_rx_frame *f=&e->rx_queue[e->rx_head];
            size_t plen=GWIR_RX_FIXED_BYTES+(size_t)f->count*4;
            if (cap < GWIR_HEADER_SIZE+plen) return -1; /* Do not lose the frame */
            uint8_t *p=response+GWIR_HEADER_SIZE;
            gwir_write_u32be(p,f->capture_id);
            gwir_write_u32be(p+4,f->carrier_hz);
            gwir_write_u16be(p+8,f->duty_permille);
            gwir_write_u16be(p+10,f->count);
            gwir_write_u32be(p+12,f->gap_after_us);
            for (uint16_t i=0; i<f->count; ++i) gwir_write_u32be(p+16+(size_t)i*4,f->durations_us[i]);
            /* Non-destructive READ. On a dropped TCP connection the phone can
               re-read exactly this frame. Only RX_ACK removes it. */
            return gwir_encode(response,cap,GWIR_RX_DATA,m.sequence,p,(uint32_t)plen,response_size);
        }
    case GWIR_RX_ACK: {
        if (m.payload_len != 4) return status_reply(response,cap,m.sequence,GWIR_BAD_REQUEST,response_size);
        uint32_t id=gwir_read_u32be(m.payload);
        if (!id) status=GWIR_BAD_REQUEST;
        else if (e->rx_count && e->rx_queue[e->rx_head].capture_id==id) {
            e->rx_head=(uint8_t)((e->rx_head+1u)%GWIR_MAX_FRAMES_PER_BUTTON);
            --e->rx_count;
            e->last_ack_id=id;
        } else if (e->last_ack_id==id) {
            /* Idempotent ACK, e.g. response lost after server dequeued frame. */
        } else status=GWIR_NO_CAPTURE; /* stale/incorrect token: never pop */
        return status_reply(response,cap,m.sequence,status,response_size);
    }
    default:
        return status_reply(response,cap,m.sequence,GWIR_UNSUPPORTED,response_size);
    }
}
