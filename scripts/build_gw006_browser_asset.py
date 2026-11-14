#!/usr/bin/env python3
"""Reproducible source-only Web UI embed for GW006/AmebaD; never flash.

Inlines the tested GWAS WebCrypto module and remote controller, computes exact
SHA-256 CSP hashes and produces a gzip C array. Does not relax any release
source or real KM4 ELF gate.
"""
import argparse
import base64
import gzip
import hashlib
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / "web/gw006_remote.html"
PROTOCOL = ROOT / "web/gw006_protocol.mjs"
UI = ROOT / "web/gw006_remote_ui.js"
TARGET = ROOT / "firmware/core/gwir_dashboard_asset.c"

def assemble():
    html = TEMPLATE.read_text(encoding="utf-8")
    protocol = PROTOCOL.read_text(encoding="utf-8")
    ui = UI.read_text(encoding="utf-8")
    if html.count("<script>/*GWIR_INLINE_SCRIPT*/</script>") != 1:
        raise ValueError("Missing or multiple script slots")
    if re.search(r"(?m)^\s*import\s", ui) or re.search(r"(?m)^\s*import\s", protocol):
        raise ValueError("Remote asset must be self-contained")
    expr = r"\bexport\s+\{[^}]+\};?\s*$"
    protocol, count = re.subn(expr, "", protocol, count=1, flags=re.S)
    if count != 1 or re.search(r"\bexport\b", protocol):
        raise ValueError("Unexpected ES module export structure")
    js = protocol.rstrip() + "\n" + ui.rstrip() + "\n"
    html = html.replace("/*GWIR_INLINE_SCRIPT*/", js, 1)
    styles = re.findall(r"<style>(.*?)</style>", html, flags=re.S)
    scripts = re.findall(r"<script>(.*?)</script>", html, flags=re.S)
    if len(styles) != 1 or len(scripts) != 1:
        raise ValueError("Unexpected inline execution points")
    if any(x in html for x in ["<script src=", "http://cdn", "eval(", "new Function("]):
        raise ValueError("External execution forbidden")
    if re.search(r"(?<![A-Za-z_])(localStorage|sessionStorage)\\s*(?:\\.|\\[)", js):
        raise ValueError("Secret persistence forbidden")
    def csp_hash(blob):
        return "'sha256-" + base64.b64encode(hashlib.sha256(blob.encode("utf-8")).digest()).decode() + "'"
    csp = (
        "default-src 'none'; base-uri 'none'; form-action 'none'; "
        "frame-ancestors 'none'; object-src 'none'; "
        f"script-src {csp_hash(scripts[0])}; "
        f"style-src {csp_hash(styles[0])}; "
        "connect-src 'self'; img-src 'none'"
    )
    blob = html.encode("utf-8")
    compressed = gzip.compress(blob, compresslevel=9, mtime=0)
    if gzip.decompress(compressed) != blob:
        raise AssertionError("gzip round-trip")
    if len(blob) > 160_000 or len(compressed) > 60_000:
        raise ValueError("Asset too large")
    return blob, compressed, csp

def c_file(compressed, csp):
    lines = ["/* Generated from pinned web source by build_gw006_browser_asset.py.",
             " * Do not manually edit: CSP hashes bind the JS and stylesheet bytes.",
             " * HTML/HTTP alone DOES NOT provide a secure context or device credentials.",
             " */", '#include "gwir_dashboard_asset.h"', "",
             "const uint8_t gwir_dashboard_gzip_html[] = {"]
    for i in range(0, len(compressed), 24):
        lines.append("    " + ",".join("0x%02x" % b for b in compressed[i:i+24]) + ",")
    lines += ["};", "const size_t gwir_dashboard_gzip_len = sizeof(gwir_dashboard_gzip_html);",
              'const char gwir_dashboard_csp[] = "'+csp+'";', ""]
    return "\n".join(lines)

def main():
    p = argparse.ArgumentParser()
    actions=p.add_mutually_exclusive_group(required=True)
    actions.add_argument("--check", action="store_true")
    actions.add_argument("--write", action="store_true")
    a=p.parse_args()
    plain, zipped, csp=assemble()
    generated = c_file(zipped,csp)
    if a.write:
        if not TARGET.exists():
            raise ValueError("Pinned SDK source archive must be extracted before writing")
        TARGET.write_text(generated, encoding="utf-8")
        print("GW006_WEB_CSP_ASSET_GENERATED", TARGET)
    print("GW006_BROWSER_ASSET=PASS",len(plain),"raw",len(zipped),"gzip",
          hashlib.sha256(plain).hexdigest(), "HTTPS_REQUIRED_NO_FLASH")

if __name__ == "__main__":
    main()
