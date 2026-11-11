# REV9 test report — 11 November 2026

| Test | Result | Actual scope |
|---|---|---|
| `make test` with GCC | PASS | Portable C protocol, IR engines, TX/RX shims, mock TCP localhost |
| `make clean && make CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined' test` | PASS | C tests instrumented with ASan and UBSan |
| `make test-kotlin` | PASS | Kotlin/JVM core for 96 keys and five frames, no APK |
| `make arm-smoke` | PASS | Clang cross-target relocatable objects with test stub for Cortex-M23 and Cortex-M33 |
| `make test-sdk-integration` | PASS | 11 C objects compiled in a simulated SDK-shaped environment; dry-run, idempotence, rollback, collision refusal |
| Full SDK KM0/KM4 compilation/link | NOT RUN | Vendor SDK checkout unavailable locally due DNS restrictions |
| Flash/IR test on GW006 | NOT RUN | OpenBeken 1.18.321 preserved |

**Note:** The mock SDK uses a minimal `ameba_soc.h` compatible with selected API signatures. Passing the mock does not establish that all source files link with the real SDK, fit flash/RAM, or run on WBRG1. The native probe workflow is manual and has not run.
