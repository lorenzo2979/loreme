#ifndef GWIR_CAPTURE_BUILDER_H
#define GWIR_CAPTURE_BUILDER_H
#include "gwir_engine.h"

/* Feed demodulated MARK/SPACE timings from a serialized RX worker task.
   A sufficiently long SPACE finishes one frame and is recorded as gap_after_us.
   Actual carrier/duty CANNOT be read from most common demodulating IR receivers;
   pass the user-selected playback estimate and label it as such in the app. */
typedef struct {
    gwir_engine *engine;
    uint32_t carrier_guess_hz;
    uint16_t duty_guess_permille;
    uint32_t frame_gap_threshold_us;
    uint16_t count;
    uint8_t last_was_mark;
    uint8_t malformed;
    uint32_t durations[GWIR_MAX_PULSES_PER_FRAME];
    uint32_t completed;
    uint32_t rejected;
} gwir_capture_builder;

void gwir_capture_builder_init(gwir_capture_builder *b, gwir_engine *e,
                               uint32_t carrier_guess_hz, uint16_t duty_guess_permille,
                               uint32_t frame_gap_threshold_us);
/* Return 1 when a complete frame is queued, 0 on ordinary input, -1 on error.
   A gap of threshold or longer terminates a frame. The gap is NOT appended
   to pulse durations and is not carrier-modulated. */
int gwir_capture_builder_feed(gwir_capture_builder *b, int is_mark, uint32_t duration_us);
void gwir_capture_builder_reset(gwir_capture_builder *b);
/* Call from serialized RX worker only after the receiver has observed at least
   idle_gap_us of continuous SPACE and no more transitions. Completes the final
   frame even when the decoder emits no separate long-space edge before RX
   is disarmed. Returns 1 when captured, 0 when nothing pending, -1 on error.
   The idle gap is not part of the payload durations.
 */
int gwir_capture_builder_finish_idle(gwir_capture_builder *b, uint32_t idle_gap_us);
#endif
