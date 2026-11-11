# REV16 — APConfig provisioning validation (11 November 2026)

The GW006 APConfig access point is **OPEN and temporary** by user choice. It is
activated solely by the PCB button long press (5 s), and closes after 5 min
or successful provisioning. No AP password, login, or token.

REV16 adds a **strict bounded parser of application/x-www-form-urlencoded POST
body** (`ssid` and `password` only) and a callback-based stage/verify/commit/
rollback abstraction. The existing STA credentials must not be overwritten
until a staged profile has been verified. No plaintext password must be
written to logs. Reject malformed percent escapes, duplicates and extra keys.
The physical APConfig listener must bind only to the AP interface and be
active only inside the temporary session; it is **not implemented here**.
The AP-only HTTP server, DHCP, secure persistent storage, STA restoration,
full flash layout, real GPIO pin verification and device testing are pending.

**Do not expose this parser as an unauthenticated LAN endpoint.**
The separate IR command TCP protocol remains authenticated HMAC-SHA256;
its designated port is **42424**, not the APConfig HTTP port.

Sources are SDK-integrated but unactivated: no NVS writes, no HTTP server and
no Wi-Fi changes are performed merely by linking this code.
