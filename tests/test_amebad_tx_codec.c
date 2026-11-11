#include "gwir_amebad_tx_codec.h"
#include "gwir_protocol.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t pulses[GWIR_MAX_PULSES_PER_FRAME];
static uint32_t words[GWIR_MAX_PULSES_PER_FRAME];

int main(void) {
    const uint32_t sample[4] = {9000u, 4500u, 560u, 1690u};
    uint32_t got[4] = {0};
    assert(gwir_amebad_encode_tx(sample, 4u, 38000u, got, 4u) == 0);
    assert(got[0] == (GWIR_AMEBAD_IR_CARRIER_FLAG | 342u));
    assert(got[1] == 171u);
    assert(got[2] == (GWIR_AMEBAD_IR_CARRIER_FLAG | 21u));
    assert(got[3] == 64u);
    for (unsigned i = 0; i < 4; ++i) assert((got[i] & GWIR_AMEBAD_IR_END_FLAG) == 0);

    assert(gwir_amebad_encode_tx(NULL, 4u, 38000u, got, 4u) == -1);
    assert(gwir_amebad_encode_tx(sample, 0u, 38000u, got, 4u) == -1);
    assert(gwir_amebad_encode_tx(sample, 4u, 24000u, got, 4u) == -1);
    assert(gwir_amebad_encode_tx(sample, 4u, 500001u, got, 4u) == -1);
    assert(gwir_amebad_encode_tx(sample, 4u, 38000u, got, 3u) == -1);
    uint32_t bad[2] = {500u, 0u};
    got[0] = 98765u;
    assert(gwir_amebad_encode_tx(bad, 2u, 38000u, got, 4u) == -1);
    assert(got[0] == 98765u);
    bad[1] = 1u; /* too short to represent at 38kHz */
    assert(gwir_amebad_encode_tx(bad, 2u, 38000u, got, 4u) == -2);
    assert(got[0] == 98765u);

    for (unsigned i = 0; i < GWIR_MAX_PULSES_PER_FRAME; ++i) pulses[i] = 30000000u;
    assert(gwir_amebad_encode_tx(pulses, GWIR_MAX_PULSES_PER_FRAME, 500000u,
                                 words, GWIR_MAX_PULSES_PER_FRAME) == 0);
    assert((words[0] & GWIR_AMEBAD_IR_COUNT_MASK) == 15000000u);
    assert((words[GWIR_MAX_PULSES_PER_FRAME - 1u] & GWIR_AMEBAD_IR_CARRIER_FLAG) == 0);
    assert(gwir_amebad_encode_tx(pulses, GWIR_MAX_PULSES_PER_FRAME + 1u,
                                 38000u, words, GWIR_MAX_PULSES_PER_FRAME) == -1);
    puts("PASS AmebaD TX encoder: 38kHz timing, alternating MARK/SPACE, FIFO flags, bounds, 2048 samples, no partial output");
    return 0;
}
