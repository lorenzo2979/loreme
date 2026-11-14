#include <assert.h>
#include <stdio.h>
#include "gwir_sdk_bootstrap.h"
#include "gwir_amebad_board_pins.h"
static gwir_sdk_boot_config cfg;
static unsigned calls;
static int accept;
static unsigned ap_commissions;
/* Host stub: no GPIO, Wi-Fi or task creation. Proves the startup path
 * calls the isolated AP service even when auth/STORAGE are unprovisioned. */
int gwir_sdk_apconfig_commission(void) { ++ap_commissions; return -1; }
const gwir_sdk_boot_config *gwir_gw006_board_port_config(void) { return &cfg; }
int gwir_sdk_boot_register(const gwir_sdk_boot_config *got) {
    assert(got==&cfg);++calls;return accept;
}
int gwir_sdk_board_prepare(void);
static int rd(void *c,uint32_t o,void*d,size_t n)
{(void)c;(void)o;(void)d;(void)n;return -1;}
static int wr(void *c,uint32_t o,const void*d,size_t n)
{(void)c;(void)o;(void)d;(void)n;return -1;}
static int er(void *c,uint32_t o)
{(void)c;(void)o;return -1;}
static gwir_fj_io io={0};
int main(void) {
    assert(gwir_sdk_board_prepare()==-2 && !calls && ap_commissions==1u);
    cfg.provider=(void*)&cfg;cfg.ir_pipeline=(void*)&cfg;
    cfg.ir_config=(void*)&cfg;cfg.saved=(void*)&cfg;
    cfg.store=(void*)&cfg;cfg.store_io=&io;
    cfg.pulse_scratch=(void*)&cfg;cfg.upload_scratch=(void*)&cfg;
    assert(gwir_sdk_board_prepare()==-2 && !calls);
    io.read=rd;io.program=wr;io.erase_sector=er;
    /* Execute actual GW006 pin-profile metadata, not a mock of validation.
     * A passing profile alone does not authorize the red-LED electrical net. */
    {
        gwir_board_pins pins = gwir_board_pins_candidate();
        assert(gwir_board_pins_validate(&pins) == 0);
        assert(pins.ir_tx != pins.ir_rx && pins.ir_rx != pins.ap_button);
        pins.red_led = pins.ir_tx;
        assert(gwir_board_pins_validate(&pins) != 0);
    }
    accept=-77;
    assert(gwir_sdk_board_prepare()==-77 && calls==1 && ap_commissions==3u);
    puts("PASS GW006 KM4: AP commissioning, safe IR pin metadata and storage-required boot; NO HARDWARE");
    return 0;
}
