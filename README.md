# GW006 IR Universal — REV15 APConfig OPEN (source draft)

## Open APConfig specification (user confirmed)

- Button held 5 seconds -> OPEN Wi-Fi APConfig. **No AP password, no captive-portal login or access token.**
- AP closes after 5 minutes or successful saving. Existing STA credentials preserved.
- Wi-Fi adapter uses `RTW_SECURITY_OPEN`, `NULL` password, length `0`.
- Regular IR network HMAC authentication is a separate feature and is unchanged.
- **Not functional on GW006 yet:** DHCP, HTTP provisioning service, physical PB23 approval, and board recovery not implemented. Do not flash.

# GW006 IR Universal Native V2.0 — REV11 half-duplex controller and SDK integration

**Date:** 11 Nov 2026. Portable C engine + Kotlin Android core, **not a flashable Realtek firmware nor an APK**.

## New in REV12

- Portable C GWAS v1 HMAC-SHA256 envelope around the complete GWIR v3 message, with signed direction, per-boot nonce and monotonic anti-replay counter.
- Fail-closed authenticated dispatcher: tampered, unsigned or replayed packets cannot invoke TX; HMAC backend is a mandatory injected callback, **not yet implemented for the actual Realtek board**.
- Kotlin/JVM companion protocol using Java's crypto API; cross-language bit-for-bit test against C/OpenSSL; no hard-coded keys.
- Authenticator and dispatcher injected as two additional source objects into native KM4 SDK project, no Wi-Fi activation, no actual key provisioning.
- See `docs/REV12_GWAS_SECURITY.md`. **Never expose raw GWIR over LAN or flash this prototype.**

## New in REV11

- Native/portable half-duplex controller: explicit RX/TX mode arbitration, nonblocking FIFO pump,
  end-of-transmission detection, inter-frame gap, timeout and fault recovery.
- **Hardware authorization fails closed**: the SDK-backed adapter returns `board_ir_ready=0`
  until a physical PCB/pinmux/IRQ review is completed. No pin changes, no driver activation.
- Additional tests simulate FIFO backpressure, clock wrap, lost pins, failed hardware stop,
  and prevent TX while RX is active.
- Vendor SDK native build to be checked in CI; no GW006 flashable image.

## New in REV10

- Added Realtek RX interrupt-status bridge `gwir_amebad_rx_irq.{c,h}`. It drains the 32-word HW FIFO on RX counter timeout (not only the FIFO threshold interrupt), forwards errors to the software queue, flushes corrupt FIFO after overflow, and clears pending flags.
- `gwir_amebad_rx_irq_bind()` merely binds a queue in RAM; **no IRQ registration, clock, pinmux, transmission, Wi-Fi or flash happens by default**. Registering and enabling the real IRQ is reserved for a future board-specific module after the GW006 pinmux is electrically verified.
- `gwir_sdk_integration_banner()` checks native link reachability of this bridge with an invalid-argument call that performs no hardware operations.
- Added mock-hardware interrupt status tests and expanded SDK mock build from 11 to 12 compiled objects. New module also included in ARM smoke test.
- **Do not flash**: linkable RX IRQ handling is not yet a complete, commissioned IR transceiver nor an authenticated Wi-Fi application.
- See `docs/REV10_RX_IRQ_INTEGRATION.md`.

## New in REV9

- Created `scripts/integrate_amebad_sdk.py`: **idempotent** KM4 integration into the official AmebaD SDK source tree, no board programming.
- Added `firmware/sdk/gwir_sdk_diag.c` as a non-hardware startup/linkage probe. It does not configure the IR peripheral, pins, Wi-Fi or TCP.
- Added simulated SDK build tests with 11 C objects, rollback, duplicate-application and collision prevention.
- Added manual GitHub Actions native SDK build probe `.github/workflows/sdk-native-rev9.yml`, which needs the full project published and will upload logs only.
- Full vendor SDK cannot be cloned in this environment, so **native KM0/KM4 linking is still unverified**; see `docs/SDK_INTEGRATION_REV9.md`.


## New in REV8

- Added native-API-shaped **IR RX FIFO** bridge, 512-event ISR queue, task-level conversion from Realtek RX counter ticks to pulse durations and frame-end detection.
- Added explicit overflow/error discard + idle resynchronization: invalid captures are rejected instead of being silently stored.
- Tested active-low/high reception, Realtek vendor function shim, queue wrap, RX errors and bounded FIFO reads.
- Host GCC, Clang ASan/UBSan and Kotlin/JVM tests pass. Cross-compiled **ten ARM relocatable objects**, 5 modules each for Cortex-M23 and M33, using test SDK stub. No linked SDK firmware.
- See `docs/TEST_RESULTS_REV8.md` and `firmware/realtek/RX_PORTING_GUIDE_REV8.md`. Original OpenBeken remains unchanged.


## Added in REV7

- Added 32-slot native TX FIFO streaming pump and Realtek SDK function adapter; test-only minimal `ameba_soc.h` stub verifies API-shaped builds, not vendor SDK compatibility.
- Verified 1 through 2048 pulse words with FIFO refill, last-packet flag and FSM-idle completion under simulation.
- Compiled portable Realtek components for **Cortex-M23/KM0** and **Cortex-M33/KM4** ARM EABI objects with Clang 17; not linked, no firmware image.
- Audited WBRG1 pad definitions from Tuya; PA18 and PA19 are UART0 pads, **not documented native IR pins**. Do not rewire or flash.
- See `docs/TEST_RESULTS_REV7.md`, `docs/TUYA_WBRG1_PIN_AUDIT_REV7.md`, and `CHECKPOINT_GW006_REV7.txt`.

## Already included from REV5

1. **Non-destructive RX_READ:** the GW006 now returns the same capture until Android sends `RX_ACK` with its capture ID. A Wi-Fi disconnect no longer deletes an unread frame in the host simulation.
2. **RX_ACK** requires the correct ID. Incorrect IDs never pop; repeated last ACK returns success. RX_ARM refuses to clear pending captures. The wire protocol was bumped to **3** (the *project* is still Native V2.0).
3. **Kotlin/JVM Android core** for GWIR v3, storing 96 buttons with up to five IR packets per button in CRC-checked files, and a staging inbox used **before** ACK, with repeated-capture detection.
4. Local TCP mock tests now prove RX_READ → disconnect → reconnect → re-read the same capture → ACK; full GCC and Clang sanitized tests pass.

## New in REV6

- Added portable Realtek native IR FIFO TX word encoder and tests.
- Verified the official AmebaD SDK contains `rtl8721d_ir.h`; a past missing-header error came from an incomplete/different SDK tree.
- Verified vendor IR pinmux example pairs and flagged incompatibility/unverified PA18/PA19 wiring.
- See `docs/REALTEK_SDK_VERIFIED.md` and `docs/TEST_RESULTS_REV6.md`. **No device firmware build yet.**

## Architecture and limits

- Actual board: RSH-GW006, WBRG1 / RTL8721CSM; **keep OpenBeken 1.18.321 unchanged** on it.
- Android app (not yet built) will own TV 36, Clima 14, Decoder 32, Speaker 14 = 96 button assignments and nonvolatile code storage.
- Firmware target would only do Wi-Fi and hardware IR RX/TX, with up to five temporarily queued IR frames (2048 durations each per current **protocol design limit**, not verified on chip).
- Proposed AP 192.168.4.1 is **not implemented**.
- Existing SDK example/probe workflows are for remote auditing; **they have not run in this conversation**, and no exact SDK directory is available locally.
- `android/core` is portable Kotlin/JVM, **not** a complete Android Studio app and not an APK. `context.filesDir` is the intended Android storage location (requires app integration and tests on actual Android; uses java.nio.file, API 26+).
- **Security is still absent:** CRC is not authentication. No LAN/AP exposure, no flash, no automatic TX retries. Device reset still loses unacknowledged RAM captures.

## Source directories

- `firmware/core`: C framing, stream parsing, IR capture builder and RX ACK engine.
- `firmware/realtek/PORTING_GUIDE.md`: remaining HAL + Wi-Fi port.
- `android/core`: `GwirV3.kt`, `AndroidCodeStore.kt`, `AndroidCaptureInbox.kt`.
- `android/tests`: offline Kotlin/JVM tests.
- `tests/`: fake HAL, TCP localhost socket integration, malformed requests tests.
- `.github/workflows/`: host core tests and optional SDK audit/probe (NOT an actual device firmware build).
- `docs/PROTOCOL.md`, `docs/TEST_RESULTS_REV5.md`, `docs/STATUS.md`: technical results and open issues.

## Immediate next engineering milestone

Acquire and audit the exact Realtek `ameba-rtos-d` SDK build tree in a remote environment, confirm actual IR API symbol locations, pinmux and FreeRTOS integration, and implement a native HAL and secured LAN transport. **Do not represent host tests as a tested GW006 image.**
