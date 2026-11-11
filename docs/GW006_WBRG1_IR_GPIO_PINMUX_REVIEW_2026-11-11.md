# GW006 / WBRG1 RTL8721CSM — IR GPIO and PinMux review
Date: 2026-11-11. **Documentation only, no changes to board firmware.**

## Answer
General GPIOs can usually be configured as input/output in software; the **native hardware IR peripheral cannot be assigned arbitrarily to any GPIO**. The AmebaD peripheral is routed to documented PinMux alternatives. A software-driven TX carrier is an unvalidated fallback requiring jitter and timing tests.

## Confirmed references
- Tuya WBRG1 module pinout: https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz
- Realtek AmebaD raw IR TX example: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_tx_led/src/main.c
- Realtek AmebaD raw IR RX example: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_rx_learning/src/main.c
- Realtek AN0400 Ameba-D Infrared Radiation chapter 25: table 25-1.
- Realtek Ameba Arduino pin-function chart documents PA25 IR_TX, PA26 IR_RX, PB23 IR_TX, PB22 IR_RX; PB31/PB29 functions require source reconciliation.

## Native IR options from SDK sample
- `PA25` TX / `PA26` RX — both module pins listed by Tuya (PA25, PA26 alias REQ). **Preferred hardware candidate only if PCB accessibility/trace tests confirm**.
- `PB23` TX / `PB22` RX — module pins exist but OpenBeken observations on GW006 identify PB23 **button** and PB22 **LED**. Treat occupied; DO NOT repurpose on board.
- `PB29` and `PB31` — third option exists in example, but SDK commented lines say PB29 TX / PB31 RX, while Realtek AN0400 and Ameba Arduino charts show PB31 TX / PB29 RX. **SOURCE CONFLICT**: do not assign either until reconciled. PB29 is not identified as an exposed WBRG1 module pad in Tuya datasheet; PB31 is labeled analog audio output (AOUTP_L).

## Legacy ex-ZS3L pads
- Previously approved *board-location* notes identify, from bottom of ZS3L right-hand pads, first pad vacant, second tentative RX, third tentative TX; labels `PA18/PA19` were hypotheses.
- Tuya module datasheet explicitly lists `PA18 = UART0_TX` and `PA19 = UART0_RX`. They are **not documented as native IR TX/RX options**.
- A software IR strategy on UART pads is conceptually possible if GPIO output/input can be safely routed, but no deterministic timing, carrier modulation, RX capture, board trace or pin direction has been verified. Do not equate UART's TX/RX naming with IR TX/RX.

## Next checks before committing GPIO configuration
1. Keep OpenBeken 1.18.321 intact.
2. Verify board traces or continuity from GW006 accessible pads to **WBRG1 PA25 and PA26** (module pins per official Tuya pinout) with board unpowered.
3. Verify PB23 button pull polarity and PB22 LED, and voltage boundaries (3.3 V only for GPIO).
4. Confirm receiver supply/signal inversion and IR LED transistor driver, and test transmit carrier/receive timing with external measurement.
5. Validate backup/recovery and native pinmux only after hardware evidence.
No firmware image flashing, no live pinmux writes authorized by this document.
