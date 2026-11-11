#include "gwir_capture_builder.h"
#include <limits.h>
#include <string.h>

void gwir_capture_builder_reset(gwir_capture_builder *b) {
    if (!b) return;
    b->count=0;
    b->last_was_mark=0;
    b->malformed=0;
}
void gwir_capture_builder_init(gwir_capture_builder *b, gwir_engine *e,
                               uint32_t carrier, uint16_t duty, uint32_t gap_us) {
    if (!b) return;
    memset(b,0,sizeof(*b));
    b->engine=e;
    b->carrier_guess_hz=carrier;
    b->duty_guess_permille=duty;
    b->frame_gap_threshold_us=gap_us;
}
int gwir_capture_builder_feed(gwir_capture_builder *b, int is_mark, uint32_t duration_us) {
    if (!b || !b->engine || !duration_us || duration_us>30000000u ||
        b->frame_gap_threshold_us<1000u || b->frame_gap_threshold_us>30000000u) return -1;
    is_mark = !!is_mark;
    /* Reject an already invalidated frame until a long idle gap. */
    if (!is_mark && duration_us>=b->frame_gap_threshold_us) {
        int result=0;
        if (b->count && !b->malformed) {
            result=gwir_engine_capture(b->engine,b->carrier_guess_hz,
                        b->duty_guess_permille,b->durations,b->count,duration_us);
            if (result==0) { ++b->completed; result=1; }
            else { ++b->rejected; result=-1; }
        } else if (b->malformed) { ++b->rejected; result=-1; }
        gwir_capture_builder_reset(b);
        return result;
    }
    if (b->malformed) return -1;
    /* Ignore idle space before a first mark. */
    if (!b->count) {
        if (!is_mark) return 0;
        b->durations[0]=duration_us;
        b->count=1;
        b->last_was_mark=1;
        return 0;
    }
    if (b->last_was_mark==(uint8_t)is_mark) {
        /* Adjacent equal levels must merge without overflow. */
        uint32_t old=b->durations[b->count-1];
        if (old>30000000u-duration_us) { b->malformed=1;return -1; }
        b->durations[b->count-1]=old+duration_us;
        return 0;
    }
    if (b->count==GWIR_MAX_PULSES_PER_FRAME) {b->malformed=1;return -1;}
    b->durations[b->count++]=duration_us;
    b->last_was_mark=(uint8_t)is_mark;
    return 0;
}

int gwir_capture_builder_finish_idle(gwir_capture_builder *b, uint32_t idle_gap_us) {
    if (!b || !b->engine || idle_gap_us < b->frame_gap_threshold_us ||
        b->frame_gap_threshold_us < 1000u || b->frame_gap_threshold_us > 30000000u ||
        idle_gap_us > 30000000u) return -1;
    if (b->malformed) {
        ++b->rejected;
        gwir_capture_builder_reset(b);
        return -1;
    }
    if (!b->count) return 0;
    int rc = gwir_engine_capture(b->engine, b->carrier_guess_hz,
                                b->duty_guess_permille, b->durations,
                                b->count, idle_gap_us);
    if (rc == 0) ++b->completed;
    else ++b->rejected;
    gwir_capture_builder_reset(b);
    return rc == 0 ? 1 : -1;
}
