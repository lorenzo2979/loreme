# CHECKPOINT CP-06 — GW006 REV10
Date: 2026-11-11.
REV9 official AmebaD native KM0/KM4 build succeeded in https://github.com/lorenzo2979/loreme/actions/runs/43043803556.
REV10 adds native-API-shaped RX IRQ status/service code, hardware error handling, idle FIFO tail draining, mock tests and linker reachability probe.
No actual GW006 pinmux, IRQ registration, Wi-Fi service, network authentication or flash; OpenBeken 1.18.321 unchanged.
Local GCC, Clang ASan/UBSan, Kotlin/JVM, ARM smoke and mock SDK (12 objects) PASS.
REV10 official SDK build must be checked after this commit; native build success is not physical board compatibility.
Source ZIP created in conversation: GW006_IR_NATIVE_V2_REV10_RX_IRQ_SDK_SOURCE.zip (69 files, 90386 bytes);
SHA256 a9c51d9e71d67c7198f162ce55c51e54c1244a6abb4254aa1af0df3f5972ef5a.
Do not flash.
