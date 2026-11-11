# REV6 test results — 11 Nov 2026

Locally executed on host, not a Realtek device:

- `make clean test` using host GCC C11: **PASS** test_engine, test_stream, test_amebad_tx_codec, TCP loopback mock.
- `make test-kotlin` using locally installed Kotlin/JVM: **PASS** protocol version 3, 96-button persistent storage simulation, five frames, ACK staging and CRC checks.
- `make clean && make CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer' test`: **PASS**, including new codec.
- `git ls-remote https://github.com/Ameba-AIoT/ameba-rtos-d.git HEAD`: **FAILED** due to DNS access restrictions in container, not due to SDK absence.
- Native ARM KM0/KM4 compile and final firmware link: **NOT EXECUTED**; missing SDK/toolchain and integration.

New codec test expectations:
- 9000us/4500us/560us/1690us at 38kHz encode to `[342 | MARK, 171, 21 | MARK, 64]` clock units.
- 2048 consecutive maximum-duration 30,000,000us samples at 500kHz encode without integer overflow.
- A duration of 1us at 38kHz is **not representable** in one normal carrier clock; encoder rejects it.
- Validation failures never partially overwrite caller's word buffer.

**Limitations:** these tests cannot verify real TX/RX pins, physical waveforms, Wi-Fi, Realtek IRQ timing, security, Android APK integration, power-loss recovery, or flash safety.
