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

## Critical board-level discovery: PA25 is probably already wired (2026-11-11)
- **Do NOT treat PA25 as a free pin.** An owner of the same RSH-GW006 gateway, in the OpenBeken conversion thread, documents: `PA25 = red LED`, `PB22 = blue WiFi LED`, `PB23 = PCB button`. The original board's OpenBeken configuration in the thread maps `"25": "LED;1"`, `"54": "WifiLED;0"`, `"55": "Btn_n;0"`. See: https://www.elektroda.com/rtvforum/topic4127578.html (post #1 and #12, 24–26 June 2025).
- PA25 therefore is **not presumed available for native IR TX**, even if IR TX mux supports it. Using it without isolating the LED circuit may load or interfere with the 38-kHz carrier. Merely disabling a LED in software does not make the PCB electrical connection disappear.
- WBRG1 `PA26` is labeled `REQ` / `PTA_REQ` in Tuya's datasheet and **may** have a routed board trace or Zigbee-radio-coexistence circuit. Its actual occupation on this particular board is **unknown**. A removed ZS3L module does not prove its former nets are isolated or free.
- Prior advice identifying PA25/PA26 as "preferred" must be interpreted as *peripheral-mux candidates*, not *confirmed vacant GPIOs*. Both require hardware verification before any soldering, IR pinmux write or flashing.
- Test with GW006 totally **unpowered**: identify module PA25 and REQ/PA26 solder contacts using the **module pad** diagram (the Tuya datasheet's chip Pin 40/39 are IC pin numbers, not WBRG1 castellated contact numbers); check continuity from each to the red-LED resistor, former ZS3L interface pads, and headers; inspect both PCB sides. If the red LED is connected to PA25, consider isolation only after a board-specific review and backup plan, not just GPIO configuration.
- Current status: not verified electrically on the user's PCB; retain OpenBeken 1.18.321 and do not flash.

## User-tested PCB trace discovery — PA26 / P2 uppermost pad (2026-11-11)
- **USER CONFIRMED by physical multimeter testing:** WBRG1 GPIO **PA26 / REQ** is **free** on the user's GW006 PCB. The user has located its connection at **P2, first pad at the top** (using the user's PCB orientation).
- This is a **user-performed electrical check**, not an independent lab measurement. Treat it as a verified-by-user candidate for **IR hardware RX**; final RX receiver wiring, power, signal polarity, IRQ and IR timing remain untested.
- Preserve P2 orientation with a future photo before soldering; no firmware flash or live pinmux change is authorized by this result.
- **Next pin to verify: PA25** for hardware IR TX. A matching GW006 board report associates it with the red LED, so determine its actual circuit/occupancy before choosing a modification.
