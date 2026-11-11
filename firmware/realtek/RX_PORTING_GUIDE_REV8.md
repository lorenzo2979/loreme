# RX hardware integration boundary — REV8

The REV8 code is compile-tested with **only an `ameba_soc.h` test stub**. The routines below are NOT wired to a working device SDK build. No real GPIO pin assignment has been approved.

## Required board-specific integration

- Confirm the actual module's RX native-IR pad and electrical receiver polarity. PA18/PA19 are Tuya WBRG1 UART0 pads; do **not** assume they can use `PINMUX_FUNCTION_IR`.
- Configure the Realtek IR peripheral in RX mode using a real SDK `IR_InitTypeDef`, setting sampling clock, threshold, filter and trigger behavior for the measured receiver. The official RX example uses 10 MHz as a starting sample rate, not a verified setting for this GW006.
- Create one globally persistent `gwir_engine`, one RX queue and one RX worker: **do not allocate the large engine (~50 KB) on a small task stack**.
- In the **real IR ISR**, check the RX status bits, drain `IR_GetRxDataLen`/`IR_ReceiveData` into the bounded queue using `gwir_amebad_rx_vendor_irq_drain(&queue)`; if a hardware overflow or counter error is indicated, call `gwir_amebad_rx_irq_error(&queue)`; for the configured inactivity threshold call `gwir_amebad_rx_irq_idle(&queue, threshold_us)` **after** draining the current FIFO. Clear the relevant real SDK interrupt pending flags; do not mask RX forever.
- In a **dedicated FreeRTOS task**, call `gwir_amebad_rx_worker_step(&worker, budget)` frequently; configure `lock_ir_irq` and `unlock_ir_irq` to mask/unmask only the IR interrupt when copying one queue event. This does not implement a lock with generic task mutexes inside the ISR.
- Hardware IR is **half-duplex**. Coordinate RX disable and TX initialization through the same serialized service task; do not let TX reconfigure peripheral while RX ISR is active.
- A real firmware build must enforce timeouts, pin/clock checks, frame limits, hardware readiness, queue lifetime, and Wi-Fi/TCP authorization. Test the actual hardware with known remote controls before considering flash.

## Format

- Realtek RX word bit31: detected signal HIGH/LOW; remaining 31 bits: counter ticks at the configured sampling clock.
- The RX worker maps a `mark_level` (usually 0 for demodulated active-low IR receiver) and turns `ticks` into integer microseconds, rejecting zero/nonrepresentable values. This interpretation is **not yet measured on the board**.
- It reconstructs alternating MARK/SPACE durations and detects a completed frame from a long SPACE or an explicit idle interrupt. A dropped/invalid partial frame is **never saved**.
- `gwir_amebad_rx_worker_step` returns number of completed captures; failures are recorded in `invalid_events`, `builder.rejected` and queue `overflows` counters. A negative return indicates API misuse, not an event-level HW error.

## Limits and remaining safety

The bounded software queue has 512 events; `sizeof(gwir_amebad_rx_queue)` should be measured on target for RAM budgeting. A 10-MHz sample setting and 30-ms idle gap are examples, not proven timing on the GW006 board. The `GWIR` protocol already supports five capture packets but there is NO secure production transport yet.

This is **not** a complete firmware and it must not be flashed.
