# CHECKPOINT CP-08 — GW006 IR Universal
Date: 2026-11-11 (Europe/Rome)

## Authoritative repository / validated revision
- Repository: https://github.com/lorenzo2979/loreme
- REV15 OPEN APConfig sources committed: 8d6a271ff0d61f9b44b62b9cb0755cdd7717a006
- REV15 KM4 integration/link fix committed: ad8fcd80a8b878e46519a79970753597ad01b2d7
- Final REV15 GitHub native build: https://github.com/lorenzo2979/loreme/actions/runs/43065278974
- GitHub native result: SUCCESS, KM0 and KM4 successful, APConfig Wi-Fi adapter linked.
- Prior failed REV15 run 43063872550: missing gwir_apcfg_session_start at link time because SDK integrator omitted gwir_apconfig_session.c; fixed by adding session and Wi-Fi source to SDK integration plus host mock tests.

## User-confirmed requirements
- Hardware: GW006 RSH-GW006, Tuya WBRG1 / RTL8721CSM.
- PCB button observed in OpenBeken as PB23; pin must be electrically validated before enabling firmware GPIO.
- Hold button for 5 seconds (debounced) to request temporary APConfig.
- APConfig is OPEN Wi-Fi: no WPA/password, no web login or token.
- SSID GW006-IR-SETUP (proposed), setup IP 192.168.4.1 (planned, not active).
- APConfig timeout 5 minutes or after completed provisioning; preserve existing STA credentials and restore on failure.
- Normal app/firmware IR TCP port 42424 (fixed in intended design, not active yet).
- DuckDNS settings belong in Android app; no implemented DDNS network service yet.
- Android remotes: TV 36, Clima 14, Decoder 32, Speaker 14 = 96 buttons.
- 5 recorded IR packets per key; authenticated GWAS/HMAC normal IR transport, separate from open APConfig.

## What is technically working
- Host tests of GWIR framing, RX ACK, TX/RX FIFOs, bus arbitration, HMAC, 96-button Kotlin core, button debounce and session semantics.
- REV15 vendor SDK native KM0/KM4 build success, logs-only CI; software is not tested on GW006 PCB.

## REV16 work prepared locally (not yet on GitHub)
- Portable form body parser for SSID/password, bounded URL decoding and rejection of invalid/duplicate fields.
- Stage/verify/commit/rollback contract: do not replace working Wi-Fi profile until verified.
- Host tests, SDK mock integration (22 sources), Kotlin/JVM, ARM compile smoke and Clang sanitizer checks passed.
- Archive: GW006_IR_UNIVERSAL_REV16_APCONFIG_PROVISIONING_SRC.zip, provided in conversation.
- **Not yet** official vendor SDK CI build for REV16.

## Remaining before a board-ready firmware
- AP-only HTTP provisioning server + DHCP + IP 192.168.4.1 implementation and session/timeouts wired to actual button.
- Secure persistence with verified rollback and board Wi-Fi association.
- Live TCP HMAC service on 42424; Android complete app and DuckDNS configuration.
- Native IR pinmux and physical TX/RX trace audit; module PA18/PA19 are UART, not confirmed native IR. PB23 button conflicts with vendor IR TX example.
- Full SDK firmware image boot layout, backup and safe recovery procedure validated on actual WBRG1/GW006 hardware.
- **DO NOT FLASH**. Original GW006 retains OpenBeken 1.18.321.


## Additional confirmed network settings: Search WiFi and DHCP/static IP
Confirmed by user on 2026-11-11, after CP-08 initial creation. Mandatory for upcoming APConfig page and Android Config UI:
- Include a visible **"Cerca Wi-Fi / Search WiFi"** action. It scans nearby Wi-Fi networks, lists SSID, signal and security type, and fills the selected SSID; support manual SSID for hidden networks and a refresh button. Check radio scan/AP concurrency on real hardware.
- **IP configuration selector: DHCP (default) or Static IP** for the normal GW006 station/network, NOT for the temporary setup AP (which should remain 192.168.4.1).
- DHCP auto-obtains IP address, subnet mask, gateway and DNS via router; static mode reveals required IPv4 address, mask, gateway, primary DNS and optional secondary DNS.
- Validate IPv4/mask/gateway/DNS and reject malformed entries, overlapping or unsafe settings before attempting network changes; do not overwrite known-good STA configuration unless connectivity is verified.
- Persist Wi-Fi network choice and DHCP/static mode together transactionally; revert to previous working config on a failed network switch. Retain existing credentials and network settings if APConfig times out or is closed.
- Keep the chosen connection mode visible in the Android Config page and accessible from the 5-minute open APConfig session; do not expose stored Wi-Fi passwords for reading.
- Open APConfig remains button-triggered (5 sec) with 5-minute timeout, no login/WPA password, used exclusively for setup; normal GWIR TCP port stays 42424 with HMAC authentication.
- **Status:** requirements recorded only. Search UI, scan backend, static-IP apply and real AP HTTP portal are not yet verified/implemented on the physical GW006; no flash.
See docs/REV17_WIFI_SCAN_DHCP_STATIC_REQUIREMENTS.md.

## Requisito confermato: MQTT esclusivamente per Home Assistant
- MQTT è una integrazione opzionale dedicata **solo a Home Assistant**; niente EMQX Cloud o altri broker cloud obbligatori.
- Per Home Assistant si configura il broker MQTT locale (es. Mosquitto integrato nel sistema HA), host/IP, porta, username e password, senza credenziali hardcoded.
- Dashboard web autonoma GW006 e app Android opzionale continuano a usare direttamente la connessione LAN, **senza dipendenza da MQTT o da Home Assistant**.
- Se Home Assistant/MQTT non sono configurati o disponibili, acquisizione, memoria e trasmissione IR locali devono continuare a funzionare.
- MQTT deve essere disattivabile e idealmente disattivato per impostazione iniziale; collegamento e discovery/entità di Home Assistant da implementare e verificare, non attivi oggi.
- Il servizio TCP IR resta sulla porta 42424, separato da MQTT; APConfig resta temporaneo e soltanto per la configurazione.

## User-confirmed LED assignments (2026-11-11)
- **Blue LED = Wi-Fi connection status**, existing GW006 GPIO **PB22**. Suggested behavior: blinking while connecting, steadily on while STA is connected, off when disconnected (subject to actual board polarity).
- **Red LED = temporary APConfig indicator**, existing GW006 GPIO **PA25**. On only while the open APConfig session is truly active; off as soon as it closes on saved provisioning or after 5-minute timeout; if AP start fails, must not falsely indicate active.
- **PCB button PB23** continues to start APConfig after five-second hold. IR TX PB31 and IR RX PA26 do not use these LED pins.
- Prior OpenBeken matching-board settings suggest PA25 `LED;1` (likely active high) and PB22 `WifiLED;0` (likely active low); treat polarity as candidate and verify on this hardware before driving pins. Do not enable GPIO outputs until the firmware's board hardware gate is validated.
- Distinguish STA Wi-Fi status from APConfig state; both indicators may be on together when STA+AP is active. This is a requirement, **not** a hardware-test claim.
