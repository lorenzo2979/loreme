# GW006 / Tuya WBRG1 — hardware IR pinmux verification checkpoint

Verified sources:
- Tuya official WBRG1 module datasheet (updated 2026-02-09):
  https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz
- AmebaD official SDK raw-IR examples:
  https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_rx_learning/src/main.c
  https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_tx_led/src/main.c

## Verified module facts
Tuya WBRG1 uses RTL8721CSM and provides 4 MB PSRAM and 8 MB flash on the module (not a validated GW006 firmware partition layout). Module UART0_TX is **PA18**, UART0_RX is **PA19**. Neither is listed as a native IR pinmux pair by the referenced SDK examples.

The AmebaD SDK IR examples document these alternative TX/RX function pairs:
- PA25 TX / PA26 RX
- PB23 TX / PB22 RX
- PB29 TX / PB31 RX

Tuya's WBRG1 datasheet lists PA25, PA26, PB22, PB23 and PB31 among exposed/module pins. That **does not establish whether the GW006 PCB routes, exposes, or leaves any of these pads available**. Existing OpenBeken observations identified PB22 as LED and PB23 as button on a particular GW006; they should be treated as occupied until physically verified.

## Consequences
- The prior ex-ZS3L pad hypothesis PA19 TX / PA18 RX is NOT confirmed as a native hardware IR route; these are module UART pins. It may require a software-based IR backend or a different verified physical route.
- The SDK's IR_Init, IR_Cmd, RX IRQ and TX FIFO routines may compile and link but **cannot transmit or receive via arbitrary GPIO pins** without valid pinmux routing.
- Never connect arbitrary wires, reassign boot/UART pins or enable IR IRQ/pinmux based solely on this document.
- On-device TX/RX must remain fail-closed until a measured board pad-to-chip trace, voltage checks, receiver polarity, TX transistor circuit, backup/recovery and safe test process are completed.
- No device flashing; keep OpenBeken 1.18.321.

This checkpoint only records sourced facts. It is NOT a substitute for a GW006 PCB continuity test or hardware test.
