#include "gwir_amebad_tx_codec.h"
#include "../core/gwir_protocol.h"

int gwir_amebad_encode_tx(const uint32_t *durations_us,
                         uint16_t pulse_count,
                         uint32_t carrier_hz,
                         uint32_t *out_words,
                         size_t out_capacity)
{
    if (!durations_us || !out_words || pulse_count == 0 ||
        pulse_count > GWIR_MAX_PULSES_PER_FRAME ||
        carrier_hz < 25000u || carrier_hz > 500000u ||
        out_capacity < pulse_count) {
        return GWIR_AMEBAD_ENCODE_INVALID;
    }

    /* Validate all entries before writing any output (all-or-nothing). */
    for (uint16_t i = 0; i < pulse_count; ++i) {
        const uint32_t us = durations_us[i];
        if (us == 0u || us > 30000000u) {
            return GWIR_AMEBAD_ENCODE_INVALID;
        }
        const uint64_t cycles = ((uint64_t)us * carrier_hz) / 1000000u;
        if (cycles == 0u || cycles > GWIR_AMEBAD_IR_COUNT_MASK) {
            return GWIR_AMEBAD_ENCODE_UNREPRESENTABLE;
        }
    }
    for (uint16_t i = 0; i < pulse_count; ++i) {
        const uint32_t cycles = (uint32_t)(((uint64_t)durations_us[i] * carrier_hz) / 1000000u);
        out_words[i] = cycles | ((i & 1u) == 0u ? GWIR_AMEBAD_IR_CARRIER_FLAG : 0u);
    }
    return GWIR_AMEBAD_ENCODE_OK;
}
