#include <assert.h>
#include <stdio.h>
#include "gwir_sdk_bootstrap.h"
#include "gwir_amebad_board_pins.h"
#include "gwir_wbrg1_flash_port.h"
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
static gwir_fj_io authorized_io={0};
static unsigned nor_attach_count;
static int nor_authorized;
/* Fake flash boundary ONLY for deterministic host validation. The real
 * adapter is compiled into the Realtek image with approval default OFF. */
int gwir_wbrg1_flash_port_attach(gwir_fj_io *out) {
    ++nor_attach_count;
    if (!out || !nor_authorized) return -7;
    *out = authorized_io;
    return 0;
}
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
    /* Fully provided but UNAPPROVED board must be blocked before runtime. */
    assert(gwir_sdk_board_prepare()==-4 && calls==0 && nor_attach_count==1u);
    /* Approval is a synthetic host-test flag; never granted by firmware. */
    authorized_io=io;
    nor_authorized=1;
    io.region_size=authorized_io.region_size+4096u;
    assert(gwir_sdk_board_prepare()==-5 && calls==0 && nor_attach_count==2u);
    io.region_size=authorized_io.region_size;
    accept=-77;
    assert(gwir_sdk_board_prepare()==-77 && calls==1 &&
           nor_attach_count==3u && ap_commissions==5u);
    puts("PASS GW006 KM4: physical AP gate, pin policy, exact NOR adapter, fail-closed boot; NO DEVICE");
    return 0;
}
