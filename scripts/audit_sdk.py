#!/usr/bin/env python3
"""Offline read-only inventory of the officially cloned Realtek AmebaD SDK.
Does not modify SDK, compile firmware or create installable binaries.
"""
import argparse
import json
from pathlib import Path
import subprocess

P = argparse.ArgumentParser()
P.add_argument('sdk', type=Path)
P.add_argument('--output', type=Path, default=Path('sdk_audit.json'))
a = P.parse_args()
root = a.sdk.resolve()
if not (root / 'component').is_dir() or not (root / 'project').is_dir():
    raise SystemExit('NOT THE AMEBAD SDK: component/ and project/ are required')

needles = ['IR_SendBuf', 'IR_ReceiveBuf', 'IR_Init', 'IR_Cmd', 'IR_RX', 'IR_TX', 'IR_Learn']
scan_extensions = {'.h', '.c', '.cpp', '.hpp'}
hits = {n: [] for n in needles}
file_count = 0
byte_count = 0
candidate_files = []
for p in root.rglob('*'):
    if not p.is_file() or p.is_symlink():
        continue
    file_count += 1
    try:
        byte_count += p.stat().st_size
    except OSError:
        continue
    rel = p.relative_to(root).as_posix()
    if 'ir' in p.name.lower() or 'infrared' in rel.lower():
        candidate_files.append(rel)
    if p.suffix.lower() not in scan_extensions or p.stat().st_size > 800_000:
        continue
    try:
        raw = p.read_bytes()
    except OSError:
        continue
    for n in needles:
        if n.encode('ascii') not in raw:
            continue
        for line_no, line in enumerate(raw.decode('utf-8', errors='replace').splitlines(), 1):
            if n in line:
                hits[n].append({'file': rel, 'line': line_no, 'snippet': line.strip()[:160]})
                if len(hits[n]) >= 50:
                    break
try:
    commit = subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'], text=True, stderr=subprocess.DEVNULL).strip()
except (OSError, subprocess.CalledProcessError):
    commit = None
result = {
    'sdk': 'Ameba-AIoT/ameba-rtos-d',
    'source': 'https://github.com/Ameba-AIoT/ameba-rtos-d',
    'commit': commit,
    'file_count': file_count,
    'total_bytes_excluding_symlink': byte_count,
    'likely_ir_files': candidate_files[:500],
    'ir_symbols': hits,
    'project_gcc_release': str((root / 'project/realtek_amebaD_va0_example/GCC-RELEASE').is_dir()).lower(),
    'previously_missing_header_rtl8721d_ir_h': any(p.endswith('/rtl8721d_ir.h') for p in candidate_files),
    'note': 'Read-only source audit; DOES NOT establish successful device build or IR hardware operation.'
}
a.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
print('SDK commit:', commit)
print('Scanned files:', file_count)
print('IR-related files:', len(candidate_files))
for n, entries in hits.items():
    print(n, 'matches:', len(entries))
print('Saved:', a.output)
