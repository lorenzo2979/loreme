# GW006 IR Universal — Search WiFi + DHCP/Static IP requirements
Date: 2026-11-11. Confirmed user requirement for physical-button APConfig and authenticated browser dashboard. The Android app is not required in the final release (decision 2026-11-14).

## Configuration UI (no unnecessary password for temporary AP)
- Setup network: GW006-IR-SETUP, visible open AP, started by pressing the PCB button for 5 seconds and closed on success or after 300 seconds. Portal is intended at http://192.168.4.1; neither a working HTTP portal nor on-device networking is claimed.
- User-confirmed 2026-11-14: **DIRECT PHONE-TO-GW006 CONNECTION**. No APConfig network password, no portal login, no home router or internet needed to enter APConfig. Select GW006-IR-SETUP on the phone and open http://192.168.4.1. Do not conflate the optional password for a selected home Wi-Fi with an APConfig password. Sensitive home Wi-Fi passwords and DuckDNS tokens must not be submitted over an unprotected HTTP channel on the open AP; use securely protected provisioning before accepting them.
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

## Browser-only APConfig, physical button, and DuckDNS
- APConfig must remain activatable exclusively by the GW006 physical PCB button; do not remove this capability.
- Browser APConfig must offer: **Cerca Wi-Fi** (scan with signal/security, SSID selection/manual entry), password, **DHCP (default) or IP statico**, and **DuckDNS**.
- Static STA configuration requires IPv4, subnet mask, gateway, primary DNS and optional secondary DNS. Validate and rollback on unsuccessful connection; never confuse the static STA address with the setup AP address.
- DuckDNS fields in APConfig: enable toggle, DuckDNS hostname/subdomain, personal token entry, explicit test/update, last-update status and bounded retry on network changes. After setup, settings must also be editable from the authenticated web dashboard.
- A DuckDNS token is a secret: do not log or echo it, and do not submit it over the old OPEN AP via cleartext HTTP. A protected provisioning channel is mandatory before transmitting the token. Do not assume DuckDNS implies Internet reachability or port-forwarding.
- No Android application is a required firmware component. Preserve ordinary authenticated IR protocol where useful, but the user-facing controls are served directly from the GW006 web interface.
- This is the **required behaviour**, not an assertion that native Wi-Fi scan, DHCP/static profile, protected setup or DuckDNS network updates have already passed hardware tests.

## Security and hardware gate
- No always-on open AP: button-only with 5-minute deadline, no web login or AP password by explicit user design. Show setup data only, never an unauthenticated control endpoint.
- A nearby user may view or alter traffic during this open session; do not display stored credentials. Preserve password in transient buffers only and scrub where practical.
- PB23 was observed as PCB button in OpenBeken but is still not physically verified, and may conflict with native IR TX pinmux.
- This specification does **not** mean scan, DHCP/static application, HTTP server, firmware flash or hardware behaviour are implemented or tested. Preserve OpenBeken 1.18.321 until recovery and pinmux verification.
