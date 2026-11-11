# GW006 REV15 - OPEN APConfig confirmed

User choice: APConfig Wi-Fi deliberately **OPEN**, without WPA/WPA2 password, without login and without an access token. This applies ONLY to the temporary setup hotspot, not normal IR command authentication.

- Trigger: PCB button pressed continuously for 5 s (REV14 logic).
- Access: nearby phone connects to visible unencrypted AP.
- Timeout: 300000 ms (5 min), or stop immediately on successful committed settings.
- Default: no AP at boot. Existing STA credentials must not be erased.
- Realtek API: `wifi_start_ap(ssid, RTW_SECURITY_OPEN, NULL, len, 0, channel)` with `RTW_MODE_STA_AP`.
- Provisioning handler should bind ONLY to AP network and expose ONLY setup actions. No remote commands or existing Wi-Fi passwords should be readable. No portal login or authentication.
- Privacy trade-off: anyone in Wi-Fi range during the 5-minute setup window can connect, read transmitted data and potentially change configuration. Limit exposure by physical-button activation and closing promptly.
- Current status: OPEN-AP adapter implemented; actual DHCP, HTTP server, STA credentials persistence and PB23 board verification remain pending. No board flash.
