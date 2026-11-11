# Realtek SDK verification — REV6 / 2026-11-11

## Source of truth

Official SDK repository: https://github.com/Ameba-AIoT/ameba-rtos-d

Verified paths on the vendor repository default branch (observed with GitHub connector):
- `component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h`: **exists** and declares `IR_Init`, `IR_SendBuf`, `IR_ReceiveBuf`, `IR_GetTxFIFOFreeLen`, `IR_GetRxDataLen`, `IR_ReceiveData`, interrupts and FIFO helpers.
- `component/soc/realtek/amebad/fwlib/ram_common/rtl8721d_ir.c`: native IR functions.
- `component/soc/realtek/amebad/fwlib/include/rtl8721d_pinmux.h`: definitions including `PINMUX_FUNCTION_IR` and `_PA_18`/`_PA_19` (their existence does NOT imply IR mux routing).
- `project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_tx_led/src/main.c`: example native IR TX.
- `project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_rx_learning/src/main.c`: example native IR RX learning.
- `project/realtek_amebaD_va0_example/GCC-RELEASE/project_lp` and `project_hp`: KM0 and KM4 build roots.

IR example pin groups in `Board_IR_Init`:

| TX | RX | GW006 board caveat |
|---|---|---|
| PA25 | PA26 | PA25 previously associated with red LED; board access unknown |
| PB23 | PB22 | PB23 button and PB22 blue LED previously reported |
| PB29 | PB31 | physical access on GW006 not confirmed |

Previously approved **PA19 TX / PA18 RX** is a board wiring proposal, **not confirmed native IR pinmux**. Do not use it with `Pinmux_Config(..., PINMUX_FUNCTION_IR)` until vendor mux mapping and board pad access have been verified. GPIO/software-timer IR would need a different HAL and independent timing benchmarks.

## Native FIFO mapping evidence

Vendor header defines:
- `IR_TX_DATA_CARRIER_MASK` as bit31 (mark with carrier).
- `IR_TX_DATA_END_MASK` as bit30 (last word flag).
- `IR_TX_CLK_NORMAL` selection as zero in bits29..28 (carrier cycles).
- `IR_TX_DATA_TIME` as lower 28 bits.
- TX/RX FIFO size 32 entries each.

Vendor NEC encoder `component/common/drivers/ir/protocol/ir_nec_protocol.c` converts microseconds to carrier cycles using integer truncation. `IR_SendBuf` sets the final END flag when `IsLastPacket` is true, so portable REV6 codec intentionally leaves END flag unset.

## New REV6 source

`firmware/realtek/gwir_amebad_tx_codec.{h,c}` converts the portable GWIR microsecond pulse array into native TX FIFO words (no vendor source copied). It validates all samples before writing output, refuses zero-cycle pulses, and accepts at most 2048 timings. The resulting conversion is **host-tested only**; it has not been run against vendor hardware and does not prove carrier accuracy or PWM availability.

`tests/test_amebad_tx_codec.c` covers NEC-like 38kHz values, alternating MARK/SPACE, 2048 samples, range validation, no END flag and all-or-nothing behavior.

## Build blockers

1. No complete Realtek SDK snapshot/commit locally and no `arm-none-eabi-gcc` toolchain in current container; network clone failed with a DNS error.
2. No native FreeRTOS/Wi-Fi TCP task, RX interrupt bridge, FIFO TX streaming/refill code, secure authentication, or firmware link integration.
3. Pinmux/PCB continuity not confirmed; no full device dump and verified recovery path.
4. No device-compatible KM0/KM4 image, APK or physical IR test.

**Do not flash this source ZIP or rename host binaries as firmware.**

Vendor sources contain their own license/copyright notices. We have not repackaged vendor source into this project.
