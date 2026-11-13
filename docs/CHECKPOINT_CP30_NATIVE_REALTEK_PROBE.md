# CP30 — Realtek CP29 complete source native build probe (NO FLASH)

This review-only branch contains exactly the SHA-256-pinned CP29 **source** ZIP and a workflow to expand it on the hosted Linux runner, audit the source manifest, run host regression tests and compile/link KM0/KM4 with the pinned AmebaD vendor SDK.

Archive SHA-256: 0c2c5ac593cc5999729fed69d70967876d79ac600f88c4284207f6596ebd24f9
Vendor SDK pin: 7569200fb8304daa61a9f09ccf439ada38d6de2e

**No firmware images are published or flashed.** A successful CI build would not validate device storage layout, board pinmux, actual hardware, boot/recovery or runtime authorization. OpenBeken 1.18.321 remains untouched. Do not merge without separate review.
