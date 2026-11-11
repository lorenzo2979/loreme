#ifndef GWIR_AMEBAD_TX_CODEC_H
#define GWIR_AMEBAD_TX_CODEC_H

#include <stddef.h>
#include <stdint.h>

/* Portable encoder for Realtek AmebaD native IR TX FIFO data.
 * It does not access registers, change pinmux, initialise IR, or flash hardware.
 * Vendor IR_SendBuf(IR_DEV, ..., IsLastPacket) adds the final END flag;
 * this converter MUST NOT set the END flag itself.
 */
#define GWIR_AMEBAD_IR_CARRIER_FLAG UINT32_C(0x80000000)
#define GWIR_AMEBAD_IR_END_FLAG UINT32_C(0x40000000)
#define GWIR_AMEBAD_IR_COUNT_MASK UINT32_C(0x0FFFFFFF)

enum gwir_amebad_encode_result {
    GWIR_AMEBAD_ENCODE_OK = 0,
    GWIR_AMEBAD_ENCODE_INVALID = -1,
    GWIR_AMEBAD_ENCODE_UNREPRESENTABLE = -2
};

/* Input durations must alternate MARK, SPACE, ... beginning at MARK.
 * Each output word has bit31=1 for MARK and bit31=0 for SPACE,
 * bits29:28=00 (normal carrier-cycle clock), bits27:0=cycle count.
 * Frequencies must be within the GWIR portable protocol range.
 * Very short timings which quantize to zero cycles are REJECTED,
 * rather than being silently lost during IR playback.
 * The vendor example rounds down (truncates) microseconds to cycles.
 */
int gwir_amebad_encode_tx(const uint32_t *durations_us,
                         uint16_t pulse_count,
                         uint32_t carrier_hz,
                         uint32_t *out_words,
                         size_t out_capacity);

#endif
