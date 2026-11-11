# Porting to Realtek RTL8721CSM (GW006)

**NOT FLASHABLE — device integration remains to be implemented and verified.**

## Correct SDK family

Use https://github.com/Ameba-AIoT/ameba-rtos-d (RTL872xCS / RTL872xD). The **new** repository `ameba-rtos` targets newer RTL8721Dx (D-plus), not the same SoC as RTL8721CSM. The older SDK builds KM0 then KM4 images. Do not rename host test binaries to `.bin`.

## Integration tasks

1. Examine the exact current SDK tree to locate the provided IR controller headers and sample projects. The official `ameba-rtos-d` repository **does contain** `component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h`; previous include failure reflected missing/different SDK files. Full SDK is still unavailable locally. See `docs/REALTEK_SDK_VERIFIED.md`.
2. Select physically accessible native IR pins and inspect board conflicts. The GW006 historically assigns PA25 red LED, PB22 blue LED, PB23 button; do not override those without intentional board changes. Verify QFN pin multiplexing and physical pads by continuity measurements before wiring.
3. Wire HAL `tx()` to native IR TX hardware, providing carrier and duty, FIFO refill/interrupt logic, all pulse durations, and inter-frame gap; use Realtek Application Note AN0400 `IR_SendBuf()` and related functions as a starting reference.
4. Wire HAL `rx_arm()` / `rx_disarm()` to hardware IR receiver, capture every mark/space duration without lossy protocol decoding and call `gwir_engine_capture()` from a serialized task, not unsynchronized ISR. Realtek AN0400 documents `IR_ReceiveBuf()` and Rx learning.
5. Implement Wi-Fi AP and/or STA on the exact SDK with a TCP service that reads the fixed 20-byte GWIR header then `payload_len` bytes, bounds checks before allocation, dispatches to `gwir_engine_process()` and returns the encoded result. AP target 192.168.4.1 is not yet implemented.
6. Add **authentication, request anti-replay, rate limits and integrity** before using the service on untrusted LANs; CRC32 is only for accidental corruption, not security.
7. Run hardware trials for NEC, unknown RAW, air-conditioner frames, five-frame sequences, carrier accuracy, pulse jitter, repeated send, and reconnect. Verify independent OpenBeken recovery path BEFORE any firmware writes.
8. Match the SDK's real bootloader / KM0 / KM4 image layout and flash mapping. DO NOT assume the layout matches OpenBeken or write a new image at a guessed offset.

The C modules in `firmware/core` compile on a host and are designed to be portable to KM4. They are **not yet linked into the Realtek SDK**, and no device-ready firmware exists.

## REV5 protocol changes for the native TCP task

- Protocol version is now **3**; do not implement any v2 Android reader against this core.
- `GWIR_RX_READ` is non-destructive: respond with `RX_DATA` containing a uint32 capture token + the 12-byte IR frame fields and durations.
- `GWIR_RX_ACK` has a 4-byte capture token; only a matching token dequeues the oldest frame. The Android app must persist the frame first.
- Never reset the queue on a new client TCP connection. Engine state should live in a device-scoped worker task while Wi-Fi reconnects; device power loss still resets it.
- No real-network use until pairing, authorization and anti-replay are implemented. In particular `TX_FRAME` retries after an uncertain response can duplicate a transmission.
