# Status — GW006 IR Universal Native V2.0, source REV9 (11 Nov 2026)

| Milestone | State |
|---|---|
| Existing OpenBeken 1.18.321 backup image identified | Yes; **not** a full device dump |
| Keep actual GW006 OpenBeken unchanged | Yes; no flash performed |
| Correct SDK family `ameba-rtos-d` RTL8721CSM | **Header `rtl8721d_ir.h` and vendor IR sample paths confirmed in official GitHub repo** |
| Core IR timing builder and five-frame queue | Host-tested |
| TCP stream parser & CRC framing | Host-tested |
| **ACK-dependent queue, with reconnect safety** | **Implemented; host-tested** |
| **Android Kotlin codec for GWIR protocol 3** | **Written; JVM-tested** |
| **Android code storage: 96 slots × up to 5 frames** | **Written; JVM-tested; not tested on device** |
| **Android temporary capture inbox (store-before-ACK)** | **Written; JVM-tested; not tested on device** |
| Portable native TX FIFO word encoder | **Implemented; host-tested, not linked to SDK** |
| Realtek TX FIFO stream manager / API bridge | **Implemented; simulated SDK-only tests. Native SDK link NOT performed** |
| Realtek SDK native RX/TX interrupt + peripheral integration | **SDK source wiring REV9 added/tested on mock; IRQ and peripherals still not configured** |
| Actual GW006 Wi-Fi AP / LAN | **Not implemented** |
| Device pairing/authentication/anti-replay | **Not implemented** |
| Android graphical app / compiled APK | **Not implemented** |
| Native KM0/KM4 build/link and `.bin` | **Not performed; full SDK cannot be cloned in this container** |
| Full flash backup, verified rollback, physical IR tests | **Not performed** |

REV9 adds a reversible SDK source-integration script and non-hardware startup diagnostic. Host test executables and *compile-only* ARM EABI object files (M23 / M33) have been produced. The ARM objects use a test shim and are NOT a linked, installable firmware image.

## Critical remaining sequence

1. Access exact full Realtek AmebaD SDK (still cannot clone from this environment). Run remote SDK audit/build probe, verify true HAL header names and pinmux for physical GW006.
2. Add FreeRTOS task, native IR RX/TX handlers with FIFO drain/refill and timing conversion; verify with unit tests and oscilloscope/logic analyzer.
3. Implement AP+STA, TCP service, **authentication and replay-safe TX**.
4. Compile and link KM0/KM4 using exact pinned SDK, compare image layout and boot chain with GW006; never guess flash offsets.
5. Build Android APK integrating the Kotlin sources and telecomando UI; test Android internal storage/fsync/rename.
6. Full device dump and documented recovery before requesting permission for any physical flash.

The storage and transport tests do **not** demonstrate Realtek IR function or reliability during a power loss.
