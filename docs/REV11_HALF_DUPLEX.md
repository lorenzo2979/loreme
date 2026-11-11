# REV11: half-duplex IR task coordinator, fail-closed on the GW006

Native AmebaD RTL8721CSM has **one** IR peripheral. Its RX capture and TX waveforms must not be active concurrently. `gwir_amebad_bus.{c,h}` enforces a serialized TX/RX state machine: OFF, RX, TX, GAP, FAULT. The result does not yet perform board-level commissioning or expose any network service.

## Functional operations

- `gwir_amebad_bus_start_rx` is only possible from OFF and only after `board_ir_ready()` returns exactly 1. When RX is on, attempts to TX are refused without touching registers. RX must be explicitly stopped; queued captures stay owned by the engine and require Android RX_ACK.
- `gwir_amebad_bus_start_tx` validates/encodes timings before any hardware calls, then configures the TX carrier/duty via callbacks and starts a nonblocking FIFO pump. `poll()` feeds at most one refill per call and verifies the hardware FSM and FIFO empty before leaving TX. It then keeps a GAP state through the requested inter-frame pause. The timeout includes the pause.
- Time calculations use unsigned 32-bit subtraction across timer rollover with bounds below 2^31 ms. A stuck TX calls hardware stop and transitions to OFF if the stop succeeds, otherwise to FAULT.
- `gwir_amebad_vendor_bus.c` compiles real Realtek calls such as `IR_Init`, `IR_Cmd`, `IR_Clear*FIFO`, `xTaskGetTickCount`, `gwir_amebad_vendor_tx_ops`, but its hardcoded `board_ir_ready` ALWAYS returns 0 on REV11. **It cannot start IR on a GW006 in the distributed version.**
- `IR_DutyCycle` is an integer divider in the SDK; requested duty that cannot be represented within 25 permille is rejected. Actual carrier timing/polarity requires physical measurement.

## Critical open blockers

1. **Do not assume PA19/PA18 carry native IR.** They are UART WBRG1 module pads. Official SDK alternative pair PA25/PA26 or PB23/PB22 may conflict with GW006 LED/button and might be inaccessible. Requires PCB trace identification before any wiring changes.
2. No validated IRQ registration, GPIO pinmux, receiver polarity/thresholds or FreeRTOS task scheduling yet. Neither `gwir_amebad_bus_poll` nor the engine is called by a production networking task.
3. No secure authenticated Wi-Fi/TCP service: CRC32 is corruption detection, not authentication. The portable Kotlin core is not a usable APK.
4. No tested device flash/restore, original OpenBeken 1.18.321 remains the device firmware.

## Tests (software only)

- `make test`: C core, RX IRQ, TX FIFO, half-duplex controller and TCP localhost.
- `make test-kotlin`: Kotlin/JVM protocol/storage unit tests.
- `make test-sdk-integration`: 14 objects in fake SDK, apply/remove rollback and collision checks.
- `make arm-smoke`: 16 ARM objects, Cortex M23 and M33, using mock ameba_soc.h.
- `make CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer' test`: sanitizer host suite.
- Real native SDK build must be confirmed with GitHub Actions and saved artifacts; **do not flash even if it succeeds**.
