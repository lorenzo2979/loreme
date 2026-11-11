# GWIR protocol v3 (REV5) — GW006 ↔ Android

**Development/test protocol only — no production authentication, no Realtek target build.**

The product name remains GW006 IR Universal Native **V2.0**; the *internal wire protocol* is version **3** because REV5 intentionally changes delivery semantics from the older v2 prototype.

## Wire framing

All integers big-endian. Request/response header: 20 bytes.

| Offset | Bytes | Meaning |
|---:|---:|---|
| 0 | 4 | `GWIR` magic |
| 4 | 1 | protocol `3` |
| 5 | 1 | command |
| 6 | 2 | reserved 0 |
| 8 | 4 | request sequence |
| 12 | 4 | payload length, <= 8208 |
| 16 | 4 | CRC32 IEEE of header bytes `0..15` then payload |
| 20 | N | payload |

TCP is a byte stream; read exact header and exact payload; reject malformed headers and bad CRC; close the session on parsing error.

## Commands

| Request | Hex | Response | Request body |
|---|---|---|---|
| HELLO | `01` | `81` HELLO_REPLY | none |
| RX_ARM | `02` | `90` STATUS | none |
| RX_DISARM | `03` | `90` STATUS | none |
| TX_FRAME | `04` | `90` STATUS | TX frame |
| RX_READ | `05` | `85` RX_DATA or STATUS | none |
| PING | `06` | `86` PONG | none |
| RX_STATS | `07` | `87` RX_STATS_REPLY | none |
| RX_ACK | `08` | `90` STATUS | 4-byte capture ID |

HELLO_REPLY 8 bytes: version 3; max frame count 5; uint16 max pulses 2048; capability bits at byte 4: bit0 queue, bit1 carrier is an *assumption* for demodulating sensors, bit2 *ACK is mandatory*.

STATUS bytes: `0` OK, `1` bad request, `2` unsupported, `3` no capture, `4` IR failure, `5` invalid range, `6` busy.

TX frame fields (12 bytes + `4 * count`): `carrierHz` uint32, `dutyPermille` uint16, `pulseCount` uint16, `gapAfterUs` uint32, followed by uint32 microsecond durations alternating MARK/SPACE starting with MARK. Allowed: `carrierHz=25000..500000`, duty 200..500, count 1..2048, gap 0..30000000, durations each 1..30000000.

RX_DATA is different: it prepends a uint32 nonzero capture ID **before** the same TX frame fields. Thus maximum RX payload is **8208 bytes**. A captured ID only identifies a frame within the current GW006 device uptime. Device reset may reuse an ID.

RX_STATS: byte0 armed; byte1 queue occupancy; byte2 capacity=5; byte3 dropped flag; bytes4..5 uint16 dropped; bytes6..7 reserved.

## Reliable receive workflow

1. Android chooses a button, arms RX, lets the physical decoder collect signals and requests RX_DISARM when capture ends (Realtek adapter must flush pending last frame first).
2. Android requests RX_READ; the GW006 returns the **same head frame** until its capture ID is acknowledged.
3. Android validates CRC and frame limits, **writes that frame to its private staging inbox with file verification**, then sends RX_ACK carrying the capture ID.
4. GW006 removes only that head after a valid ACK. Duplicate ACK of the latest acknowledged ID is harmless. Incorrect IDs never remove data.
5. Android repeats until NO_CAPTURE, then commits 1–5 staged frames to the selected button file.
6. Android must examine RX_STATS and reject the capture if dropped>0; a dropped frame cannot be repaired by an ACK.

- On Wi-Fi interruption **while the GW006 remains powered**, reconnect and RX_READ again; the unacknowledged head remains in RAM.
- An RX_ARM request while unacknowledged frames remain returns BUSY instead of clearing the queue.
- **Device reset/power loss erases the GW006 RAM queue**. REV5 does not claim protection from that loss.
- Android staging files have per-file CRC and best-effort atomic file replacement on supported Android file systems; test `filesDir` on actual Android and make backups before production.
- Android app must handle retries idempotently: if it sees a capture ID already staged with identical payload, ACK without appending it twice. Avoid token reuse across separate capture sessions/reboots.

## TX retry warning

`TX_FRAME` is **not idempotent**. If the phone loses the response, it must not blindly retry; the infrared command may already have been emitted. Production protocol will need authenticated sessions, replay protection and a dedicated TX command ID/deduplication mechanism.

## Security

CRC32 only protects against accidental corruption; **it is not authentication**. No pairing, HMAC, session authorization, anti-replay, rate limiting, secure key storage, TLS, or access control is implemented in the device firmware. The host mock binds only to `127.0.0.1`. **DO NOT expose this protocol on a real AP/LAN as it stands.**

## Memory boundary

The GW006 stores no permanent button codes. RAM queue max five frames, max 2048 durations per frame. Android owns up to 96 buttons, each with up to five frames. A demodulating RX sensor normally cannot measure the original carrier; the selected `carrierHz` is an estimate, not a measurement.
