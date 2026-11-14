#!/usr/bin/env python3
"""Add GWIR STORE_ABORT to authenticated C runtime after base STORE patches.

Use exact, audited source anchors. The operation does not touch a device or
partition. Accepts ONLY the already patched in-memory source, never guesses.
"""
import argparse
from pathlib import Path


def replace_exact(root: Path, name: str, old: str, new: str) -> None:
    p = root / name
    data = p.read_text()
    count = data.count(old)
    if count != 1 or new in data:
        raise ValueError(f"Refuse STORE_ABORT transformation: {name} matches={count}")
    p.write_text(data.replace(old, new, 1))


def apply(root: Path) -> None:
    replace_exact(root, "gwir_protocol.h",
                  "    GWIR_STORE_INFO = 14, /* payload empty; bitmap 96 button records */",
                  "    GWIR_STORE_INFO = 14, /* payload empty; bitmap 96 button records */\n"
                  "    GWIR_STORE_ABORT = 15, /* authenticated RAM staging cancel; no NOR write */")
    replace_exact(root, "gwir_auth_dispatch.c",
                  "decoded.command <= GWIR_STORE_INFO))) {",
                  "decoded.command <= GWIR_STORE_ABORT))) {")
    replace_exact(root, "gwir_saved_play_command.c",
                  "    case GWIR_STORE_DELETE:",
                  """    case GWIR_STORE_ABORT:
        /* Cancel an interrupted upload without erasing a committed record.
         * A signed and monotonically authenticated reply is mandatory. */
        if (msg->payload_len != 1u) break;
        if (!s->upload_active || button != s->upload_button) {
            status = GWIR_NOT_READY;
            break;
        }
        upload_reset(s);
        status = GWIR_OK;
        break;
    case GWIR_STORE_DELETE:""")
    replace_exact(root, "gwir_saved_play_command.c",
                  "msg->command <= GWIR_STORE_INFO)",
                  "msg->command <= GWIR_STORE_ABORT)")
    print("GW006_AUTHENTICATED_STORE_ABORT_APPLIED=PASS")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", type=Path, required=True)
    args = parser.parse_args()
    apply(args.source_dir)
