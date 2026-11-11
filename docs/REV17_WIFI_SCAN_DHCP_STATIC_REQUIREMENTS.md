# GW006 IR Universal — Search WiFi + DHCP/Static IP requirements
Date: 2026-11-11. Confirmed user requirement, for APConfig and Android Config screen.

## Configuration UI (no unnecessary password for temporary AP)
- Setup network: GW006-IR-SETUP, visible open AP, started by pressing the PCB button for 5 seconds and closed on success or after 300 seconds. Portal is intended at http://192.168.4.1; neither a working HTTP portal nor on-device networking is claimed.
- Place a prominent **Cerca Wi-Fi / Search WiFi** control on Wi-Fi configuration. On tap, start a bounded, asynchronous scan and list nearby SSIDs; show signal strength and encryption type. Selecting an item fills SSID. Offer manual SSID for hidden networks; allow Refresh. Avoid logging/storing scanned passwords.
- SDK candidate: wifi_scan_networks(rtw_scan_result_handler_t handler, void* user_data) in the official AmebaD component/common/api/wifi/wifi_conf.h. Verify that background scanning remains compatible with an active configuration AP on RTL8721CSM before enabling it.
- User enters the password for the selected home Wi-Fi (if needed). Do not include manufacturer credentials or hidden hard-coded keys.

## Network addressing
- Toggle / radio selection: **DHCP (default)** | **IP statico**.
- With DHCP, do not require manually entered IPv4 or DNS; obtain IPv4, subnet, gateway and DNS from DHCP.
- With static, show and validate **IPv4 address**, **subnet mask**, **default gateway**, **primary DNS**, and **optional secondary DNS**.
- Validate dotted IPv4 and contiguous mask, host not network/broadcast, sensible subnet/gateway consistency; reject multicast, unspecified and out-of-range/reserved addresses according to available board routing. Reject unsafe conflicts with the temporary AP subnet 192.168.4.0/24 while AP mode is active.
- Saving the network profile must be transactional: stage -> attempt connection (and validate link/gateway/DNS as feasible) -> commit; rollback on failure. Persist DHCP/static choice in nonvolatile configuration alongside the SSID. Never erase existing good credentials on entry to setup.
- 192.168.4.1 is the **temporary APConfig interface** address; static/DHCP applies to the **normal station interface**, not APConfig.

## Android + remote access
- Provide the same configuration choices on the Android app Config page, without duplicating divergent settings.
- Ordinary IR command TCP listener remains on **42424** and must be authenticated (GWAS HMAC), separate from the temporary setup HTTP surface.
- DuckDNS domain and token are configured in Android; DDNS update/network exposure is not yet implemented. DuckDNS alone does not open a router port.

## Security and hardware gate
- No always-on open AP: button-only with 5-minute deadline, no web login or AP password by explicit user design. Show setup data only, never an unauthenticated control endpoint.
- A nearby user may view or alter traffic during this open session; do not display stored credentials. Preserve password in transient buffers only and scrub where practical.
- PB23 was observed as PCB button in OpenBeken but is still not physically verified, and may conflict with native IR TX pinmux.
- This specification does **not** mean scan, DHCP/static application, HTTP server, firmware flash or hardware behaviour are implemented or tested. Preserve OpenBeken 1.18.321 until recovery and pinmux verification.
