# GW006 IR Universal — REV13 SDK HMAC-SHA256 backend

## Implemented

- firmware/sdk/gwir_sdk_hmac.[ch]: GWAS callback backed by SDK Mbed TLS mbedtls_md_hmac(SHA256).
- Output is exactly 32 bytes; 32-byte key required, bounded length, no fallback; cryptographic failure clears partial output.
- Keys are supplied only by the caller. No production key, password, Wi-Fi credential or provisioning token is embedded.
- Local test uses OpenSSL through a TEST-ONLY compatibility shim, with public known-answer vectors and replay/failure tests.
- Realtek integration copies adapter sources and header, NEVER the host OpenSSL/shim.
- No pinmux, IRQ registration, WLAN listener, AP mode or flash activated. Boot banner checks only missing-argument rejection.

## NOT implemented

- Secure device/app pairing, session provisioning, key rotation and safe storage.
- Hardware RNG, authenticated per-boot nonce exchange, durable replay policy.
- Authenticated TCP listener and FreeRTOS thread ownership.
- GW006 physical IR routing (PA18/PA19 are UART), voltage/PCB checks, backups/recovery.
- Complete Android APK and physical board validation.

The unit-test keys are public non-secret vectors, not device credentials. Do not upload live keys to a public repo.

Local checks: host GCC, Clang ASan/UBSan, Kotlin/JVM, SDK mock integration (17 C modules), ARM compile-only smoke.
Official Realtek KM0/KM4 native build must be checked after commit. This is NOT a flashable GW006 firmware.
