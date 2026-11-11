# GW006 IR Universal — REV16 Wi-Fi profile provisioning core

REV15 open APConfig has been published for native SDK build. REV16 adds
bounded form decoding and an atomic-profile **backend contract**. The
HTTP AP-only portal, DHCP, verified board pin, persistent storage and
live Wi-Fi association remain to be implemented and physically tested.

- APConfig: OPEN Wi-Fi, no password/login, hold PCB button for 5s, auto-close after 5min.
- STA: keep previous credentials until verification succeeds; never erase on a failed edit.
- Normal IR TCP: fixed port **42424**; HMAC auth, distinct from APConfig.
- DuckDNS: future Android settings integration; no enabled DDNS service here.
- No flash of GW006. Keep OpenBeken 1.18.321.

See docs/REV16_PROVISIONING_VALIDATION.md and CP-07 in GitHub.
