# REV10 — AmebaD RX interrupt bridge
Standalone IRQ handler for RTL8721CSM, built against vendor IR status/clear API.
- Services FIFO on counter-timeout even when below FIFO threshold.
- On hardware error or overflow invalidates the current frame instead of saving truncated IR.
- Software-only bind; **no IRQ registration, IR_Init, IR_Cmd, GPIO pinmux, Wi-Fi or flash by default**.
- Board-specific IRQ registration, certified physical pinmux, TX/RX half-duplex arbitration, task locks, authentication, rollback and physical tests are still missing.
- Pin PA18/PA19 of WBRG1 are UART and not a vendor-confirmed native IR pair. Do not change wiring based on examples alone.
- 38 kHz carrier used for learned RX codes is an estimate, not measured by a demodulating receiver.
Tests: make test (new test_amebad_rx_irq), make test-kotlin, make arm-smoke, make test-sdk-integration (12 SDK mock objects).
Realtek SDK references: https://github.com/Ameba-AIoT/ameba-rtos-d
This is **not** a GW006-ready firmware and must **not** be flashed.
