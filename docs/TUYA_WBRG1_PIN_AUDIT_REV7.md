# GW006 / WBRG1 native IR pin audit — 11 Nov 2026

**No PCB modification, GPIO assignment, flashing or automatic pin reconfiguration is authorized by this document.**

## Official module GPIO exposure
Tuya WBRG1 datasheet, updated 2026-02-09:
https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz

- **PA18** = TX0 (UART0 TX), not listed as a native IR pin.
- **PA19** = RX0 (UART0 RX), not listed as a native IR pin.
- **PA25** appears on module pads and has native IR TX assignment in AmebaD samples and datasheet.
- **PA26** appears on module pads as REQ (PTA) and has native IR RX assignment; repurposing it may conflict with board wiring/other firmware functions.
- **PB23** module pad (button previously identified on GW006) supports native IR TX.
- **PB22** module pad (blue LED previously identified on GW006) supports native IR RX.
- **PB31** AOUTP_L audio pad appears on the module; other end of its IR pair, PB29, is not in Tuya WBRG1 exposed pin list; use only with further verification.

## Native IR selection according to Realtek examples
Source: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_tx_led/src/main.c

- **PA25 TX / PA26 RX** (candidate to investigate on actual GW006 board, not yet approved).
- **PB23 TX / PB22 RX** (currently conflicts with button / blue LED known from GW006).
- The *example source comments* label **PB29 TX / PB31 RX**, whereas a Realtek AmebaD app note and Arduino board documentation label **PB31 TX / PB29 RX**. This is a source inconsistency. **Do not select either of these pins based solely on comments.**

Pinmux generic header `rtl8721d_pinmux.h` defines PA18, PA19 and PINMUX_FUNCTION_IR but **this does not imply that the IR peripheral can be routed to those pins**.

## Important conclusion
Previous GW006 drawing used **PA19 TX / PA18 RX**, which is the reverse of the pins' UART0 directions and **not a vendor-documented native IR pair**. The drawing remains an historical wiring reference, not an approved native IR design. Do not change existing connections from this document; first verify PCB pads, continuity, shared components, native pinmux and physical IR device voltages.

Source for M23 / M33 cores: https://www.realmcu.com/img/ipa/en_637914076404583067.pdf (RTL872xCS QFN68 datasheet). The SDK calls these KM0 and KM4, but their Armv8-M instruction sets correspond to Cortex-M23 and Cortex-M33, not Cortex-M0/M4 respectively. `scripts/cross_arm_smoke.sh` uses the corresponding Clang targets.
