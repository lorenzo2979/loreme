# REV9 — AmebaD native SDK integration (KM4, NO FLASH)

**Status:** Source-level integration complete and tested against an isolated SDK-shaped test fixture. Actual full SDK native build **not run**. The current environment cannot resolve `github.com` for git clone, and the full Realtek SDK was unavailable locally.

## Verified upstream build layout
The correct SDK is [`Ameba-AIoT/ameba-rtos-d`](https://github.com/Ameba-AIoT/ameba-rtos-d), not the later `ameba-rtos` tree for different SoCs. The following upstream file paths and anchors were inspected using the GitHub connector:
- `project/realtek_amebaD_va0_example/src/src_hp/Makefile`: builds `main.c`, uses `CSRC`, `OBJS`, source `vpath` and `COPY_RAM_OBJS`.
- `project/realtek_amebaD_va0_example/src/src_hp/main.c`: starts `vTaskStartScheduler()`.
- `project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/make/project/Makefile`: builds project submodules.
- `project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/Makefile.include.gen`: compiler and include settings.
- `component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h`: defines `IR_InitTypeDef`, `IR_StructInit`, `IR_SendBuf`, `IR_ReceiveData`, etc.
- `component/soc/realtek/amebad/fwlib/ram_common/rtl8721d_ir.c`: native driver implementation.

## How to integrate into a legitimate full SDK checkout
From this REV9 root:

```bash
python3 scripts/integrate_amebad_sdk.py --sdk /path/to/ameba-rtos-d --dry-run
python3 scripts/integrate_amebad_sdk.py --sdk /path/to/ameba-rtos-d --apply
```

The script copies **GW006-authored code only** into `project/realtek_amebaD_va0_example/src/gwir_sdk/{core,realtek,sdk}` and inserts marked REV9 blocks into the vendor `src/src_hp/Makefile` and `src/src_hp/main.c`. The original source files are backed up as `*.gw006-before-rev9`. It **does not change pinmux/bootloader/Wi-Fi/FTL** and runs only a safe startup log/codec test; native IR and networking stay **disabled**.

One can remove the inserted blocks and copied files with:

```bash
python3 scripts/integrate_amebad_sdk.py --sdk /path/to/ameba-rtos-d --remove
```

The script rejects an incomplete SDK, refuses to overwrite unowned conflicting files, is idempotent and will not overwrite existing backups.

## Test status and boundaries
- ✅ REV8 core GCC/Clang and Kotlin tests, plus ARM relocatable object compilation, inherited unchanged.
- ✅ REV9 test `make test-sdk-integration`: simulated SDK Makefile compilation of 11 C objects (including `main.c`), dry-run, repeated apply, removal, and collision refusal.
- ⏳ KM0/KM4 native SDK compilation/linking against **real** vendor headers and libraries: not yet performed.
- ⏳ Actual boot, GPIO IR mux, RX/TX timers, interrupt behavior and power budget: not tested.
- ⏳ Wi-Fi TCP authentication and application API: not implemented. **CRC32 is not authentication.**
- ⏳ APK: there is only the Android Kotlin/JVM core.

## Remote-build probe (manual)
Once REV9 source tree is checked in to a repository, the manual GitHub Actions workflow `.github/workflows/sdk-native-rev9.yml` clones the official SDK on a GitHub runner, applies the integration, attempts KM0 then KM4 full native builds and uploads **logs only**. SDK build may fail because a vendor-specific toolchain/dependencies are required; results should be judged from actual logs, not a green badge from host-only tests.

**No generated `.bin` is authorized for flashing** until WBRG1 hardware pin mappings, backup and recovery are independently verified. Leave OpenBeken 1.18.321 unchanged.
