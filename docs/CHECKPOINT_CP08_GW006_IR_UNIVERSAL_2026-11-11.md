# CHECKPOINT CP-08 — GW006 IR Universal
Date: 2026-11-11 (Europe/Rome)

## Authoritative repository / validated revision
- Repository: https://github.com/lorenzo2979/loreme
- REV15 OPEN APConfig sources committed: 8d6a271ff0d61f9b44b62b9cb0755cdd7717a006
- REV15 KM4 integration/link fix committed: ad8fcd80a8b878e46519a79970753597ad01b2d7
- Final REV15 GitHub native build: https://github.com/lorenzo2979/loreme/actions/runs/43065278974
- GitHub native result: SUCCESS, KM0 and KM4 successful, APConfig Wi-Fi adapter linked.
- Prior failed REV15 run 43063872550: missing gwir_apcfg_session_start at link time because SDK integrator omitted gwir_apconfig_session.c; fixed by adding session and Wi-Fi source to SDK integration plus host mock tests.

## User-confirmed requirements
- Hardware: GW006 RSH-GW006, Tuya WBRG1 / RTL8721CSM.
- PCB button observed in OpenBeken as PB23; pin must be electrically validated before enabling firmware GPIO.
- Hold button for 5 seconds (debounced) to request temporary APConfig.
- APConfig is OPEN Wi-Fi: no WPA/password, no web login or token.
- SSID GW006-IR-SETUP (proposed), setup IP 192.168.4.1 (planned, not active).
- APConfig timeout 5 minutes or after completed provisioning; preserve existing STA credentials and restore on failure.
- Normal app/firmware IR TCP port 42424 (fixed in intended design, not active yet).
- DuckDNS settings belong in Android app; no implemented DDNS network service yet.
- Android remotes: TV 36, Clima 14, Decoder 32, Speaker 14 = 96 buttons.
- 5 recorded IR packets per key; authenticated GWAS/HMAC normal IR transport, separate from open APConfig.

## What is technically working
- Host tests of GWIR framing, RX ACK, TX/RX FIFOs, bus arbitration, HMAC, 96-button Kotlin core, button debounce and session semantics.
- REV15 vendor SDK native KM0/KM4 build success, logs-only CI; software is not tested on GW006 PCB.

## REV16 work prepared locally (not yet on GitHub)
- Portable form body parser for SSID/password, bounded URL decoding and rejection of invalid/duplicate fields.
- Stage/verify/commit/rollback contract: do not replace working Wi-Fi profile until verified.
- Host tests, SDK mock integration (22 sources), Kotlin/JVM, ARM compile smoke and Clang sanitizer checks passed.
- Archive: GW006_IR_UNIVERSAL_REV16_APCONFIG_PROVISIONING_SRC.zip, provided in conversation.
- **Not yet** official vendor SDK CI build for REV16.

## Remaining before a board-ready firmware
- AP-only HTTP provisioning server + DHCP + IP 192.168.4.1 implementation and session/timeouts wired to actual button.
- Secure persistence with verified rollback and board Wi-Fi association.
- Live TCP HMAC service on 42424; Android complete app and DuckDNS configuration.
- Native IR pinmux and physical TX/RX trace audit; module PA18/PA19 are UART, not confirmed native IR. PB23 button conflicts with vendor IR TX example.
- Full SDK firmware image boot layout, backup and safe recovery procedure validated on actual WBRG1/GW006 hardware.
- **DO NOT FLASH**. Original GW006 retains OpenBeken 1.18.321.
