# CP30 — Native build ELF gate correction (diagnostic-only)

Latest pinned package SHA-256: `8252096eaf5728e98d6329238f5fd7f0b335c7ad953a65f9406d9ea99ab531e4`. This ZIP differs from CP29 **only** in `scripts/verify_native_km4_objects.py`, `tests/test_native_km4_objects.py` and the source manifest hashes. `gwir_dashboard_asset.o` is a read-only asset blob, so its ARM ELF32 relocatable must have allocated non-executable data with a defined bounded STT_OBJECT symbol instead of a function. Every other module still requires code and a defined function symbol.

The workflow captures byte-identical compiled ARM objects during vendor `COPY_RAM_OBJS`, prior to cleanup, allowing strict freshness/ELF checks after KM4 build. The final link map gate stays strict and may reveal pruned modules because runtime isn't activated.

NO FLASH, NO DEVICE CHANGE. OpenBeken 1.18.321 unchanged. Draft PR / do not merge.
