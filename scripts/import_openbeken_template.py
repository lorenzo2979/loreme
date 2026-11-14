#!/usr/bin/env python3
"""Read-only OpenBeken template -> safe GW006 GPIO evidence.

OpenBeken RTL8720D pin indices 0..31=PA0..PA31 and 32..63=PB0..PB31.
The conversion is source-level (OpenBeken GPIO alias mapping) and does NOT
verify physical board routes or license IR / Flash writes.
No network access. Never prints commands, passwords, or full raw template.
"""
import argparse
import json
import re
from pathlib import Path

ROLE_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
MAX_BYTES = 65536


def _unique_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("Duplicate JSON key")
        result[key] = value
    return result


def parse_template(raw: str) -> dict:
    if len(raw.encode("utf-8")) > MAX_BYTES:
        raise ValueError("Template exceeds 64 KiB")
    obj = json.loads(raw, object_pairs_hook=_unique_keys)
    if not isinstance(obj, dict) or not isinstance(obj.get("pins"), dict):
        raise ValueError("Expected OpenBeken JSON object with a 'pins' mapping")
    result = []
    for k, assignment in obj["pins"].items():
        if not isinstance(k, str) or not k.isdecimal() or str(int(k)) != k:
            raise ValueError("Pin identifiers must be canonical decimal strings")
        n = int(k)
        if not 0 <= n < 64:
            raise ValueError("Pin index outside 0..63")
        if not isinstance(assignment, str) or assignment.count(";") != 1:
            raise ValueError("Expected pin assignment in 'Role;Channel' format")
        role, channel = assignment.split(";")
        if not ROLE_RE.fullmatch(role) or not channel.isdecimal() or not 0 <= int(channel) <= 255:
            raise ValueError("Invalid GPIO role/channel")
        gpio = "PA" + str(n) if n < 32 else "PB" + str(n - 32)
        result.append({"openbeken_pin_index": n, "realtek_gpio": gpio,
                       "role": role, "channel": int(channel),
                       "physical_signal_verified": False})
    result.sort(key=lambda p: p["openbeken_pin_index"])
    chip = obj.get("chip", "")
    if not isinstance(chip, str):
        chip = ""
    # Model names/commands/SSID/password are never copied to the output.
    return {
        "source": "local OpenBeken exported template",
        "chip_family_label": chip[:40],
        "pin_mapping_source":
            "OpenBK7231T_App/src/hal/realtek/rtl8720d/hal_pins_rtl8720d.c",
        "gpio_assignments": result,
        "no_hardware_access": True,
        "no_flash_write_permission": True,
        "configuration_not_electrically_verified": True,
        "full_flash_backup_provided": False,
        "does_not_certify_safe_firmware_installation": True,
        "warning": "Pin aliases are not physical GW006 wiring evidence."
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="Locally exported OpenBeken JSON template")
    parser.add_argument("--output", type=Path, default=None,
                        help="Sanitized GPIO report JSON (default: stdout)")
    args = parser.parse_args()
    raw = args.input.read_text(encoding="utf-8")
    report = json.dumps(parse_template(raw), indent=2, ensure_ascii=False) + "\n"
    if args.output:
        args.output.write_text(report, encoding="utf-8")
    else:
        print(report, end="")


if __name__ == "__main__":
    main()
