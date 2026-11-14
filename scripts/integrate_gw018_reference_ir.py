#!/usr/bin/env python3
"""Integrate GW006 IR core into a TEMPORARY checkout of GW018-DM AmebaD SDK.

Purpose: compile a real KM4 image with GWIR IR engine/driver in its boot call
graph. NO flash, NO keys, NO authorization of GW006 pins or storage. This build
intentionally disables GW018 Zigbee TCP bridge and its OTA button conflict.
"""
from __future__ import annotations
import argparse
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROJECT = Path("project/realtek_amebaD_va0_example")
GROUPS = {
    "core": [
        "gwir_protocol.c", "gwir_engine.c", "gwir_capture_builder.c",
    ],
    "realtek": [
        "gwir_amebad_tx_codec.c", "gwir_amebad_tx_fifo.c",
        "gwir_amebad_tx_vendor.c", "gwir_amebad_rx_fifo.c",
        "gwir_amebad_rx_vendor.c", "gwir_amebad_rx_irq.c",
        "gwir_amebad_bus.c", "gwir_amebad_vendor_bus.c",
    ],
    "sdk": ["gwir_gw018_ir_boot.c"],
}
MAIN_BLOCK = """\t/* GWIR: Prepare the actual raw IR core and IRQ processing task.
\t * No IR output or listening ports before safe GW006 board validation. */
\t{
\t\textern int gwir_gw018_ir_boot_prepare(void);
\t\tint gwir_boot = gwir_gw018_ir_boot_prepare();
\t\tDiagPrintf("[GWIR REF] IR core boot=%d, pins and flash NOT enabled\\n", gwir_boot);
\t}
"""

def integrate(root: Path) -> None:
    sdk = root / PROJECT
    src = sdk / "src"
    main = src / "src_hp" / "main.c"
    make = src / "src_hp" / "Makefile"
    config = sdk / "inc" / "inc_hp" / "platform_opts.h"
    for f in (main, make, config,
              root / "component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h"):
        if not f.is_file() or f.is_symlink():
            raise ValueError(f"Not GW018-DM SDK / missing safe file: {f}")
    dest = src / "gwir"
    if dest.exists():
        raise ValueError("Refusing to overlay existing GWIR integration")

    mk = make.read_text()
    if mk.count("CSRC = $(DIR)/main.c") != 1:
        raise ValueError("GW018-DM KM4 Makefile source anchor changed")
    mk_lines = [
        "# GWIR native IR integration: source compiled and linked from KM4 main.",
        "GWIR_DIR := $(DIR)/../gwir",
        "MODULE_IFLAGS += -I$(GWIR_DIR)/core -I$(GWIR_DIR)/realtek -I$(GWIR_DIR)/sdk",
        "vpath %.c $(GWIR_DIR)/core $(GWIR_DIR)/realtek $(GWIR_DIR)/sdk",
    ]
    for group, files in GROUPS.items():
        mk_lines.append("CSRC += " + " ".join(
            f"$(GWIR_DIR)/{group}/{name}" for name in files))
    mk = mk.replace("CSRC = $(DIR)/main.c",
                    "CSRC = $(DIR)/main.c\n" + "\n".join(mk_lines), 1)

    m = main.read_text()
    if m.count("vTaskStartScheduler();") != 1:
        raise ValueError("GW018-DM scheduler anchor changed")
    start = m.find("// Init red LED", m.find("int main(void)"))
    end = m.find("if (wifi_config.wifi_ultra_low_power", start)
    if start < 0 or end < 0 or "gpio_init(&led_red, LED_RED);" not in m[start:end]:
        raise ValueError("GW018-DM PA25 red LED startup changed")
    # GW018 uses PA25 as a red LED; GW006 reserves PA25 for IR TX.
    m = (m[:start] + "/* GW006 IR reference: do not drive PA25 as GW018 LED. */\n\t"
         + m[end:])
    m = m.replace("vTaskStartScheduler();",
                  MAIN_BLOCK + "\tvTaskStartScheduler();", 1)

    opts = config.read_text()
    for symbol in ("CONFIG_EXAMPLE_SOCKET_TCP_TRX", "CONFIG_EXAMPLE_OTA_HTTP"):
        pattern = rf"(?m)^(#define\s+{symbol}\s+)1(\s*)$"
        opts, count = re.subn(pattern, lambda match: match.group(1) + "0" + match.group(2), opts)
        if count != 1:
            raise ValueError(f"Cannot disable GW018-only {symbol} safely: {count}")
    # Verify and prepare all files before modifying vendor tree.
    plan = {}
    for group, names in GROUPS.items():
        for name in names:
            source = ROOT / "firmware" / group / name
            if not source.is_file():
                raise ValueError(f"Missing source: {source}")
            plan[dest / group / name] = source
        for h in (ROOT / "firmware" / group).glob("*.h"):
            plan[dest / group / h.name] = h

    for dst, src_file in plan.items():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src_file, dst)
    make.write_text(mk)
    main.write_text(m)
    config.write_text(opts)
    print(f"GW018_GWIR_INTEGRATION=PASS; {sum(len(g) for g in GROUPS.values())} native C sources")
    print("GW018_ZIGBEE_TCP_DISABLED=1; GW018_BUTTON_OTA_DISABLED=1; GW018_PA25_LED_DISABLED=1")
    print("GW006_IR_HARDWARE_ENABLED=0; GW006_FLASH_WRITES_ENABLED=0")

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    args = parser.parse_args()
    integrate(args.sdk.resolve(strict=True))

if __name__ == "__main__":
    main()
