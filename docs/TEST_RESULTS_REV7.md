# REV7 verified build / test report — 11 Nov 2026

## Source base and implementation
- Base package: REV6 preserved.
- New C source: `firmware/realtek/gwir_amebad_tx_fifo.{c,h}` streams up to 2048 encoded IR words into an abstract 32-word TX FIFO, without enqueueing more than its reported free capacity.
- New vendor-symbol bridge: `firmware/realtek/gwir_amebad_tx_vendor.{c,h}` calls `IR_GetTxFIFOFreeLen`, `IR_SendBuf`, `IR_Cmd` and `IR_FSMRunning` from the *real* SDK once integrated.
- New **test-only stub**: `tests/stubs/ameba_soc.h` simulates SDK declarations. Never ship this as the real SDK header.
- New test `tests/test_amebad_tx_fifo.c` checks 1, 2, 31, 32, 33, 64, and 2048 words; write failures, FIFO full, END marker, start-once, and queue-vs-transmission completion.

## Actual verification
1. `make clean && make test` on GCC: **PASS** (core, stream, TX codec, TX FIFO, localhost TCP protocol).
2. `make CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined' test`: **PASS**, no reported sanitizer errors.
3. `make test-kotlin` via Kotlin compiler and JVM: **PASS** (GWIR v3, 96 slots, 5 frames, ACK).
4. `make arm-smoke`: **PASS** using Clang 17 `--target=arm-none-eabi` for **Cortex-M23 (KM0)** and **Cortex-M33 (KM4)**; produced six ARM EABI relocatable `.o` files. **The vendor bridge used a deliberately minimal SDK *test stub*.**

## Important boundaries
- No actual Realtek complete SDK linked or used to compile in this environment, no FreeRTOS board project, Wi-Fi, real hardware RX path, pinmux integration, real carrier setup, timed loop/gaps or interrupt-driven FIFO service.
- `TX_DONE` means simulated hardware says FSM idle and FIFO empty after END; still requires empirical timing/hardware tests.
- Object files are *not* firmware images and must never be flashed.
- Real network authentication/replay protection is NOT implemented. CRC alone is NOT security.
- OpenBeken on the board was not touched.

## Direct next integration steps
1. Download/use official `ameba-rtos-d` SDK and build on trusted ARM toolchain; validate actual include and symbol compatibility.
2. Validate PA25/PA26 availability against the GW006 schematic/PCB; retain or remove button/LED only after explicit hardware decision.
3. Connect TX streaming pump to a dedicated serialized device task; schedule regular refills and enforce timeout / PWM setup / final gap.
4. Implement native RX IRQ FIFO collection, conversion and frame-gap segmentation; test up to five raw frames.
5. Add network authorization and anti-replay, then Wi-Fi TCP server.
6. Only after reproducible full KM0/KM4 build, exact flash layout and complete backup/restore verification, consider any physical flash.
