#!/usr/bin/env python3
"""Read-only source inventory and call-graph audit. Never deploy/flash.
The source release is a pinned public code bundle, NOT a GW006 firmware image.
"""
from pathlib import Path
from zipfile import ZipFile
import hashlib
import re
src = Path('source_bundles/GW006_WBRG1_STORAGE_PORT_SOURCE_ONLY.zip')
expected='1a97c5f571c85224ea5df923bbf114addb3acea8bf1b65cbf35f3d653c168044'
assert hashlib.sha256(src.read_bytes()).hexdigest() == expected, 'source lock mismatch'
targets = [
 'firmware/sdk/gwir_sdk_bootstrap.c',
 'firmware/sdk/gwir_sdk_bootstrap.h',
 'firmware/sdk/gwir_sdk_secure_start.c',
 'firmware/sdk/gwir_sdk_sta_dashboard.c',
 'firmware/sdk/gwir_sdk_apconfig_port.c',
 'firmware/sdk/gwir_sdk_apconfig_wifi.c',
 'firmware/realtek/gwir_dashboard_sta_server.c',
 'firmware/realtek/gwir_dashboard_http.c',
 'firmware/realtek/gwir_amebad_board_pins.c',
 'firmware/realtek/gwir_wbrg1_flash_port.c',
 'firmware/core/gwir_apconfig_http.c',
 'firmware/core/gwir_apconfig_tcp_transport.c',
 'firmware/core/gwir_network_profile.c',
]
# Only source-code listings, no device secrets, dump contents or binary firmware.
with ZipFile(src) as z:
    names=set(z.namelist())
    for name in targets:
        print('\n### FILE',name,flush=True)
        if name not in names:
            print('MISSING from pinned source')
            continue
        lines=z.read(name).decode('utf-8').splitlines()
        print('lines',len(lines))
        if name.endswith('_bootstrap.c') or name.endswith('_bootstrap.h') or name.endswith('_secure_start.c') or name.endswith('_sta_dashboard.c') or name.endswith('gwir_dashboard_sta_server.c'):
            chunks=lines[:400]
        else:
            pat=re.compile(r'^(?:int|void|static |const |typedef |struct |\s*#include|\s*return |\s*if \(|.*(?:bind|init|start|poll|listen|serve|prepare|handle|http|wifi|auth|store|flash|board)\s*\()',re.I)
            nums=[(i+1,s) for i,s in enumerate(lines) if pat.search(s)]
            for i,s in nums[:140]:
                print(f'{i:04d} {s[:180]}')
            continue
        for i,s in enumerate(chunks,1):
            print(f'{i:04d} {s}')
