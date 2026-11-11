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

## User multimeter finding — PA25 reaches red LED positive pad (2026-11-11)
- User located **PA25** and reports a connection to the **positive/anode pad of the red LED** on their GW006 PCB. This confirms the red LED circuit is associated with the proposed IR TX GPIO; it is **not free as currently routed**.
- This is a reported board measurement. It does **not yet establish** whether PA25-to-LED positive is a direct ~0 ohm copper connection or a path through a current-limit resistor/other components; verify resistance with power completely removed.
- **DO NOT** connect IR TX electronics to PA25 yet, set IR PinMux, flash, or assume removing the LED alone electrically isolates its circuit. Check the series resistor and whether the net also ties to 3.3 V or another circuit before choosing an isolation point.
- Suggested next measurement: board unpowered, ohms from WBRG1 PA25 contact to LED positive and negative pads, and both sides of nearby LED series resistor; record readings. PA26 was independently user-tested free at P2 upper pad for IR RX.

## User confirmation: PA25 has DIRECT continuity to red LED positive pad
- User confirmed that continuity between WBRG1 **PA25** and the **red LED anode (+)** is **direct**, with no series resistor between the two measured points.
- This establishes that PA25 is already electrically connected to an LED pad, not available as-is for dedicated IR TX.
- Do not infer that removing the LED automatically frees PA25: verify the red LED cathode connection and any shared nets, any resistor and power rails before modifying anything. With the board completely unpowered, confirm whether the LED's cathode runs through a current-limiting resistor to GND and whether PA25 has any additional branches. A red LED + GPIO pad can still be fed by surrounding circuitry.
- If the LED is ultimately removed, use a separate verified transistorized IR LED driver instead of powering an IR emitter directly from PA25; signal-level and drive polarity need bench verification.
- PA26 / REQ remains user-verified free at first upper pad of P2 (candidate IR RX). No board flash or pinmux enable.

## PB31 / AOUTP_L candidate — hardware check required (2026-11-11)
- User reports GW006 has no attached microphone or speaker, making an unused audio output **plausible**, NOT electrically proven.
- WBRG1 **module contact 20 = AOUTP_L = PB31** (RTL8721CSM silicon pin 65) according to Tuya's official module datasheet: https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz.
- Authoritative Realtek **AN0400 Ameba-D, table 25-1** explicitly lists **PB31 = IR_TX** and **PB29 = IR_RX**, aligned with Realtek Arduino pin-function chart (PB31 IR_TX, PB29 IR_RX): https://device.report/m/8208322652842119c3dcee579daf4e485c48ab659a5fcde2eb8c310884ad9e05 ; https://www.amebaiot.com.cn/cn/amebad-arduino-getting%20started/.
- Important **source inconsistency**: AmebaD official SDK example contains *commented* sample lines marked PB29 TX, PB31 RX (reversed). Prior docs noted this disagreement. For selecting PB31 TX, prefer AN0400 table and Arduino pin chart, but verify native pinmux and hardware waveform with actual RTL8721CSM before treating supported.
- A board lacking speakers/microphones can still route PB31 to a resistor, capacitor, test pad, connector, removed ZS3L, ground or 3.3 V; only a visual inspection plus unpowered ohms/continuity testing can establish that the line is not shared/loaded.
- Proposed TX path **PB31** (instead of PA25, which is directly tied to red LED anode), and verified PA26 as RX. It is only a **candidate**, not a final assigned GPIO until physical measurement. Do not modify firmware, flash or solder yet.

## User multimeter confirmation — PB31 free (2026-11-11)
- User **confirmed physically tested**: `PB31` (`AOUTP_L`, WBRG1 module contact 20) is **free** on the user's GW006 PCB. This replaces the earlier unverified occupancy assumption.
- User previously physically tested `PA26` (`REQ`, module contact 11) free and located its board connection at **P2, first upper pad**.
- New proposed **native IR pin pairing** for this specific user's board: **TX = PB31**, **RX = PA26**. This pairing allows red LED on PA25, blue LED on PB22 and APConfig button on PB23 to remain untouched.
- **Important pending validation**: Realtek AN0400 documents PB31=IR_TX, but the SDK's commented sample reverses PB29/PB31 labels; confirm on native SDK/hardware which PinMux is actually driven by IR TX before enabling it. Verify user-tested GPIO's actual board net/voltage, driver stage and receiver polarity with controlled bench testing.
- User measurements are not yet a demonstrated 38-kHz carrier or successful IR receive. **Do not flash or activate pinmux** until full firmware/board recovery gate is satisfied. Existing OpenBeken remains installed.

## Final user hardware pin selection (2026-11-11)
- User explicitly selects **PB31 (WBRG1 module contact 20, AOUTP_L) for native IR TX**, accepting that soldering may need to be directly on the WBRG1 module's external castellated pad.
- **PA26 (REQ, WBRG1 module contact 11)** remains the chosen native IR RX, user-tested free; P2 upper pad is user-reported continuity location, but be aware P2 silk is marked RX1 and independent net-to-pin mapping must be reconciled before relying on that pad.
- Do not solder to an **internal bare RTL8721CSM silicon IC leg**; prefer the external WBRG1 module pad, or a verified same-net test point. Protect adjacent pads/antenna; use a strain-relieved fine insulated signal wire at 3.3 V logic level, not directly an IR LED load.
- Preserve PA25 red LED, PB22 blue LED, PB23 physical APConfig button.
- PB31=IR_TX selection still needs actual native PinMux/waveform confirmation because commented SDK examples conflict with Realtek reference table. No on-board flash until full firmware image, hardware driver and recovery have been verified.

## User-confirmed LED assignments (2026-11-11)
- **Blue LED = Wi-Fi connection status**, existing GW006 GPIO **PB22**. Suggested behavior: blinking while connecting, steadily on while STA is connected, off when disconnected (subject to actual board polarity).
- **Red LED = temporary APConfig indicator**, existing GW006 GPIO **PA25**. On only while the open APConfig session is truly active; off as soon as it closes on saved provisioning or after 5-minute timeout; if AP start fails, must not falsely indicate active.
- **PCB button PB23** continues to start APConfig after five-second hold. IR TX PB31 and IR RX PA26 do not use these LED pins.
- Prior OpenBeken matching-board settings suggest PA25 `LED;1` (likely active high) and PB22 `WifiLED;0` (likely active low); treat polarity as candidate and verify on this hardware before driving pins. Do not enable GPIO outputs until the firmware's board hardware gate is validated.
- Distinguish STA Wi-Fi status from APConfig state; both indicators may be on together when STA+AP is active. This is a requirement, **not** a hardware-test claim.
