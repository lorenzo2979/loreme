# GW006 IR Universal REV8 — test report (2026-11-11)

## What was actually added

- `firmware/realtek/gwir_amebad_rx_fifo.{c,h}`: a fixed-capacity 512-event software queue for the hardware RX FIFO. The intended producer is an IR ISR and the consumer is a serialized FreeRTOS task. RX event encoding retains the raw 32-bit value (bit31 level, low31 timer ticks).
- `firmware/realtek/gwir_amebad_rx_vendor.{c,h}`: API-shaped bridge to actual Realtek SDK symbols `IR_GetRxDataLen` and `IR_ReceiveData`. The host/cross-compile test uses the **stub**, not vendor headers.
- RX worker converts hardware ticks to microseconds using 64-bit integer arithmetic, selects configurable active-low/high MARK polarity, and calls the existing frame segmentation/ACK engine from task context only.
- A full RX software-queue discards the *entire current frame* until a later idle event. This is deliberate: it must not save truncated IR codes.
- A per-event IR interrupt masking callback protects the queue between the ISR and the worker. This callback must be implemented on target hardware before use.

## Verification actually performed

- GCC `make clean && make test`: PASS (engine, stream, TX codec, TX FIFO, RX FIFO, TCP localhost).
- Clang `make CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined' test`: PASS, no sanitizer errors reported.
- Kotlin/JVM `make test-kotlin`: PASS, no Android APK built.
- `make arm-smoke`: Clang produces ten (5 modules x 2 CPU) relocatable objects for **Cortex-M23/KM0** and **Cortex-M33/KM4** using a tiny **test** SDK header stub; NOT linked, NOT flashed.
- RX tests cover carrier estimate propagation, active-low and active-high MARK interpretation, 10-MHz timer conversion, idle completion, long-SPACE segmentation, hardware error/zero-counter rejection, queue overrun/discard/resync, 32-sample vendor FIFO drain and 300 batches testing queue index wraparound.

## What is still NOT built or tested

1. No actual `ameba-rtos-d` linker build, board pinmux/IRQ map, FreeRTOS task registration or KM0/KM4 image integration.
2. No evidence native IR can be routed through the **previous PA19 TX / PA18 RX** wiring. The original GW006 diagram refers to UART pads; it cannot be treated as tested native-IR pin assignment.
3. No safe pin reassignment, no connection/flash on the actual GW006, no validated bootloader recovery procedure.
4. RX ISR scheduling/critical-section masking and carrier/tick calibration remain to be checked under load. The 512-event queue is not a guarantee against high-frequency carrier raw waveforms.
5. No authenticated Wi-Fi/TCP service or anti-replay controls; GWIR CRC is NOT authentication. No Android APK.
6. Capture carrier is an estimate (38 kHz by default in tests), not measured from a demodulated IR receiver.

## SDK references

- Realtek IR peripheral header and RX sample: https://github.com/Ameba-AIoT/ameba-rtos-d/tree/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_rx_learning
- RX peripheral definitions: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h
- Module pinout: https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz
