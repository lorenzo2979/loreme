# REV12 — GWAS v1 authenticated transport groundwork (NOT an enabled LAN service)

The original GWIR v3 frame uses CRC32 for detection of accidental corruption; its CRC is NOT authentication. REV12 defines an independent **GWAS v1 HMAC-SHA256 envelope**, implemented in portable C (`firmware/core/gwir_auth.{h,c}`), Kotlin/JVM (`android/core/GwirAuth.kt`) and a fail-closed engine dispatcher (`firmware/core/gwir_auth_dispatch.{h,c}`). Host tests compute real HMAC-SHA256 using OpenSSL; Android tests use Java `javax.crypto.Mac` and compare a byte-for-byte C/JVM test vector. OpenSSL is a **host testing dependency only**, and no private keys or device credentials are committed.

## Binary record (all multi-byte integers big-endian)

| Byte offset | Size | Field |
|---|---:|---|
| 0 | 4 | `GWAS` magic |
| 4 | 1 | Version 1 |
| 5 | 1 | Direction 1=device request, 2=device response |
| 6 | 2 | Reserved zeros |
| 8 | 16 | **Unpredictable 128-bit device boot nonce** |
| 24 | 8 | Strictly increasing unsigned counter, from 1 through 2^63−1 |
| 32 | 4 | Full inner GWIR v3 length |
| 36 | 20..8228 | Complete inner GWIR v3 frame (header, CRC32, payload) |
| end | 32 | HMAC-SHA256 tag over **ALL preceding record bytes** |

Maximum signed record size: **8296 bytes**. Keys: per-device **32-byte secret**, never compiled into firmware. Direction and nonce are authenticated. The constant-time 32-byte MAC compare in C rejects incorrect tags, and the Android implementation uses `MessageDigest.isEqual`. The inner GWIR CRC/framing is also validated before accepting a command. An unsigned message is not accepted by the new dispatcher.

## Replay and key management requirements

- Counters increase monotonically within the same boot; separate state is maintained for commands and responses, and the state **must survive Wi-Fi/TCP reconnects**. No automatic resetting of anti-replay counters on a reconnect is safe.
- On **every actual board boot**, the hardware must generate an unpredictable new nonce using a validated cryptographic RNG. The firmware must not use a fixed/build-time nonce, a simple clock, or an unseeded PRNG.
- The Android app must authenticate the device's boot identity and use per-device keys from a properly designed pairing/provisioning procedure. Actual on-device credential provisioning, secure persistence, Android Keystore integration, nonce exchange and recovery after a lost ACK are **NOT IMPLEMENTED**.
- Authenticated replay protection does not imply confidentiality. Commands remain readable without TLS/encryption. Future transport will also require careful framing, single-client/serial-task ordering, rate limits, size limits, and suitable trusted access policies.
- Do not create a fallback to raw GWIR/CRC32 or allow a service to call `gwir_engine_process()` directly. `tests/mock_tcp_server.c` remains an **insecure localhost-only test fixture** and must never be deployed as a network listener.
- On the RTL8721CSM, a vetted mbedTLS/hardware-crypto HMAC implementation must be wired to `gwir_auth_hmac_fn` before any network use; **REV12 intentionally has no production HMAC backend**, no generated keys, no pairing, no open TCP listener, no Wi-Fi service.

## Fail-closed dispatch

`gwir_auth_dispatch` authenticates the request with direction `TO_DEVICE`, enforces replay protection, then calls the GWIR engine; responses are separately signed with direction `FROM_DEVICE`. Buffers must be allocated statically and sized at least `GWIR_MAX_MESSAGE` and `GWIR_AUTH_MAX_PACKET` to avoid a post-execution buffer-size failure. Duplicate authenticated requests are rejected before TX/ACK can be repeated. The original HAL callbacks and unsigned GWIR engine are not modified for host tests; the future production network entry point must use the new dispatcher **exclusively**.

A cryptographic failure during response signing can occur after a side effect (such as an IR transmission); therefore automatic blind retries are forbidden. A later implementation must add request IDs and a durable response/retry policy. A restarted authentication context with a reused boot nonce allows a counter reset: that is why fresh hardware RNG boot nonces and per-boot session continuity are mandatory.

## Verification performed locally

- `make test` with GCC and Clang ASan/UBSan: auth known HMAC vector (RFC 4231), encode/decode, tamper detection, wrong direction, old boot nonce, replay, oversized frame, overlap rejection, HMAC unavailable, signed dispatcher and original IR tests.
- `make test-kotlin`: full byte-for-byte cross-language HMAC test vector, direction, replay, tamper, nonce rotation and existing Android 96-slot tests.
- `make test-sdk-integration`: native-project injection into a mock SDK (16 objects), idempotence and rollback.
- `make arm-smoke`: existing Realtek IR modules build as ARM objects with a test stub. Native SDK KM0/KM4 build must be verified through GitHub Actions after publishing REV12.

**OpenBeken 1.18.321 is untouched. Board IR pinmux, backup/restore, RX polarity, Wi-Fi, and real hardware tests are still outstanding. DO NOT FLASH.**
