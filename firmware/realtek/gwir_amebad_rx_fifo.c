#include "gwir_amebad_rx_fifo.h"
#include <limits.h>

void gwir_amebad_rx_queue_init(gwir_amebad_rx_queue *q) {
    if (q) { q->rd=0u; q->wr=0u; q->used=0u; q->dropping=0u; q->overflows=0u; }
}
static int enqueue(gwir_amebad_rx_queue *q, uint8_t type, uint32_t value) {
    if (!q) return -1;
    if (q->dropping) {
        /* A producer loss makes this frame unusable. Only a known idle edge
         * can re-synchronize; never turn a truncated frame into a capture. */
        if (type != GWIR_RX_IDLE) return -2;
        type=GWIR_RX_RESYNC;
    }
    if (q->used == GWIR_AMEBAD_RX_SW_DEPTH) {
        if (q->overflows != UINT32_MAX) ++q->overflows;
        q->dropping=1u;
        return -2;
    }
    gwir_amebad_rx_event *e=&q->events[q->wr];
    e->type=type;
    e->value=value;
    q->wr=(uint16_t)((q->wr+1u)%GWIR_AMEBAD_RX_SW_DEPTH);
    ++q->used;
    if (type==GWIR_RX_RESYNC) q->dropping=0u;
    return 0;
}
int gwir_amebad_rx_irq_word(gwir_amebad_rx_queue *q, uint32_t word) {
    return enqueue(q,GWIR_RX_WORD,word);
}
int gwir_amebad_rx_irq_idle(gwir_amebad_rx_queue *q, uint32_t idle_us) {
    if (!idle_us || idle_us>30000000u) return -1;
    return enqueue(q,GWIR_RX_IDLE,idle_us);
}
int gwir_amebad_rx_irq_error(gwir_amebad_rx_queue *q) {
    return enqueue(q,GWIR_RX_ERROR,0);
}
int gwir_amebad_rx_worker_init(gwir_amebad_rx_worker *w, gwir_amebad_rx_queue *q,
                               gwir_engine *e, gwir_amebad_rx_lock l,
                               uint32_t sample_hz, uint8_t active_mark_level,
                               uint32_t carrier_guess_hz, uint16_t duty_guess_permille,
                               uint32_t gap_threshold_us) {
    if (!w || !q || !e || !l.lock_ir_irq || !l.unlock_ir_irq ||
        sample_hz<100000u || sample_hz>100000000u || active_mark_level>1u ||
        carrier_guess_hz<25000u || carrier_guess_hz>500000u ||
        duty_guess_permille<200u || duty_guess_permille>500u ||
        gap_threshold_us<1000u || gap_threshold_us>30000000u) return -1;
    w->queue=q;
    w->last_overflows=q->overflows;
    w->invalid_events=0u;
    w->decoded_words=0u;
    w->discard_until_idle=0u;
    w->lock=l;
    w->sample_hz=sample_hz;
    w->active_mark_level=active_mark_level;
    gwir_capture_builder_init(&w->builder,e,carrier_guess_hz,duty_guess_permille,gap_threshold_us);
    return 0;
}
/* Only called from the worker. There is no heap allocation or blocking inside
 * the critical section, and only one event is copied per lock cycle. */
static int take_event(gwir_amebad_rx_worker *w, gwir_amebad_rx_event *ev,
                      uint32_t *overflows) {
    int has=0;
    w->lock.lock_ir_irq(w->lock.ctx);
    *overflows=w->queue->overflows;
    if (w->queue->used) {
        *ev=w->queue->events[w->queue->rd];
        w->queue->rd=(uint16_t)((w->queue->rd+1u)%GWIR_AMEBAD_RX_SW_DEPTH);
        --w->queue->used;
        has=1;
    }
    w->lock.unlock_ir_irq(w->lock.ctx);
    return has;
}
int gwir_amebad_rx_worker_step(gwir_amebad_rx_worker *w, uint32_t max_events) {
    if (!w || !w->queue || !w->builder.engine || !max_events ||
        !w->lock.lock_ir_irq || !w->lock.unlock_ir_irq) return -1;
    int completed=0;
    for(uint32_t i=0;i<max_events;i++) {
        gwir_amebad_rx_event ev;
        uint32_t overflows=0;
        int has=take_event(w,&ev,&overflows);
        if (overflows!=w->last_overflows) {
            w->last_overflows=overflows;
            gwir_capture_builder_reset(&w->builder);
            w->discard_until_idle=1u;
        }
        if (!has) break;
        if (ev.type==GWIR_RX_RESYNC) {
            gwir_capture_builder_reset(&w->builder);
            w->discard_until_idle=0u;
            continue;
        }
        if (w->discard_until_idle) {
            if (ev.type==GWIR_RX_IDLE) {
                gwir_capture_builder_reset(&w->builder);
                w->discard_until_idle=0u;
            }
            continue;
        }
        if (ev.type==GWIR_RX_ERROR) {
            ++w->invalid_events;
            gwir_capture_builder_reset(&w->builder);
            w->discard_until_idle=1u;
            continue;
        }
        int r=0;
        if (ev.type==GWIR_RX_IDLE) {
            r=gwir_capture_builder_finish_idle(&w->builder,ev.value);
        } else if (ev.type==GWIR_RX_WORD) {
            uint32_t ticks=ev.value & GWIR_AMEBAD_RX_COUNTER_MASK;
            /* 64-bit integer scaling avoids overflow. An unrepresentable
             * near-zero duration is rejected, NOT silently rounded to 1us. */
            uint64_t us=((uint64_t)ticks*1000000u+w->sample_hz/2u)/w->sample_hz;
            if (!ticks || us==0u || us>30000000u) {
                ++w->invalid_events;
                gwir_capture_builder_reset(&w->builder);
                w->discard_until_idle=1u;
                continue;
            }
            const int is_mark=((ev.value>>31)&1u)==w->active_mark_level;
            r=gwir_capture_builder_feed(&w->builder,is_mark,(uint32_t)us);
            ++w->decoded_words;
        } else {
            ++w->invalid_events;
            gwir_capture_builder_reset(&w->builder);
            w->discard_until_idle=1u;
            continue;
        }
        if (r==1) ++completed;
        else if (r<0) {
            ++w->invalid_events;
            /* Never take a subsequent fragment as a new frame. */
            gwir_capture_builder_reset(&w->builder);
            w->discard_until_idle=1u;
        }
    }
    return completed;
}
