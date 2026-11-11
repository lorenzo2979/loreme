# REV14 — GW006 PCB button for APConfig (design and integration checkpoint)

## Agreed behavior
- Candidate hardware input: PCB button, observed as WBRG1 `PB23` in OpenBeken.
- Debounce: 50 ms.
- Long press: continuous 5,000 ms **after** the stable press edge; opens a commissioning window once per press.
- Press shorter than threshold: no action; no factory reset, erase or change of stored Wi-Fi credentials.
- Auto-close window after 300,000 ms (5 minutes), or when credentials have **actually** been committed.
- Boot with button already depressed: ignored until released and pressed again.
- Clock wrap is handled by modulo-2^32 elapsed time; call poll regularly (<1s recommended).
- Callback failure is surfaced; an AP stop failure is retried rather than treating the network as closed.

## Safety / hardware constraints
- PB23 is an observed button candidate, not yet physically electrically verified. Confirm via continuity, polarity and voltage before enabling.
- Official IR SDK examples also list PB23 as an IR TX option: **PB23 is reserved for the button, not for IR TX** in this plan. Do not configure its pinmux for IR.
- The SDK adapter compiles GPIO_ReadDataBit(_PB_23), but its board gate is hardcoded to fail closed. It does not call GPIO_Init or Pinmux_Config, and it never starts an AP.
- The AP backend is not yet implemented. Its callbacks intentionally return an error until WPA2+ device-unique credential provisioning and safe Wi-Fi rollback are built.
- Hold-to-AP state machine is not connected to a live board task. This release proves its logic, not an operating AP.
- No hardcoded AP or Wi-Fi passwords; no physical board flash. Preserve OpenBeken 1.18.321.
- The actual GW006 module and board bootloader/flash layout require independent verification.

## Test matrix
1. Debounce noise; short press; hold exactly 5 seconds.
2. Trigger once per hold; close after 5 minutes; rearm after release.
3. Backend AP start failure: no false positive; fresh physical press required.
4. AP stop failure: remains active and retries.
5. Credentials-committed callback closes AP; no Wi-Fi credential mutation in this module.
6. Boot with button held ignored; 32-bit clock wrap.
7. SDK link-only diagnostic and mock SDK build; official native KM0/KM4 GitHub workflow required to verify real SDK GPIO symbols.

**NOT FLASHABLE; no board GPIO or network activation.**
