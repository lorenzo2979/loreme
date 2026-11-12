# GW006 IR Universal — Checkpoint CP-16 (12 novembre 2026)

**Target**: RSH-GW006 / WBRG1 / RTL8721CSM. **Solo sorgenti: NON FLASHARE.** OpenBeken 1.18.321 preservato.

## Fix di compatibilità reale con l'SDK Realtek
La Realtek AmebaD SDK ufficiale in `component/common/api/network/include/lwipopts.h` definisce `LWIP_SO_SNDRCVTIMEO_NONSTANDARD 1`. Questo significa che `setsockopt(..., SO_RCVTIMEO/SO_SNDTIMEO, ...)` richiede **un int in millisecondi**, non il `struct timeval` POSIX usato dal listener CP-15.

CP-16 corregge `firmware/realtek/gwir_dashboard_sta_server.c` aggiungendo `set_socket_timeout_ms()`, con ramo `#if defined(GWIR_USE_LWIP) && defined(LWIP_SO_SNDRCVTIMEO_NONSTANDARD) && LWIP_SO_SNDRCVTIMEO_NONSTANDARD` a intero in millisecondi e ramo POSIX con `timeval`.

**Protezione fail-closed:** se il timeout sul socket in ascolto non può essere impostato, il server termina con errore `-4` senza fare `bind/listen`; se fallisce sui socket dei client, il client viene chiuso senza servire HTML.

## Evidenze su computer
Comando: `make test test-sdk-integration test-web-assets test-kotlin arm-smoke test-auth-live-tcp test-dashboard-live-tcp test-dashboard-sta-host test-sdk-sta-adapter-mock test-dashboard-lwip-abi`.

**PASS (exit 0):** regressione C e Kotlin, parser IR e GWAS HMAC/anti-replay, socket TCP reali in locale, HTTP dashboard gzip, 33 oggetti SDK simulati, ARM compile-only, test STA/AP gate.

**Nuovo test CP-16** compila lo stesso server con `GWIR_USE_LWIP=1` e shim che accetta **solo timeout int da 1000 o 2000 ms**; GET, POST negato, AP transition e chiusura corretta PASS. Con timeout forzato a fallire, listener non avviato PASS.

## Mancano
- **Build/link nativa KM0/KM4:** non effettuata. SDK e ARM toolchain non presenti nella sessione; clone pubblico indisponibile per DNS. Uno shim API è un test, non un vero build SDK.
- Task FreeRTOS e server STA non collaudati sul WBRG1, ancora disattivati di default. I comandi IR non sono accessibili da questa dashboard di sola lettura.
- Archiviazione flash permanente per 96 tasti × 5 pacchetti e API web autenticate non implementate sul chip.
- Pin TX PA25 / RX PA26 da verificare fisicamente; PA25 occupato dal LED rosso. Driver IR BC337 5V: aggiungere 1kΩ alla base, mantenere 220Ω sul LED, massa comune; mai 5V sul GPIO.
- Backup di tutta la flash e percorso di recovery hardware prima di qualsiasi flash.

## Archivio
`GW006_IR_UNIVERSAL_CP16_LWIP_TIMEOUT_SOURCE.zip` (185 file) consegnato nella chat, SHA-256 `31c6368b0f56d073c362064246737d31e5132bf56acebe6190e187aa71a56a1c`. Include log e test. Patch separata `GW006_CP16_REALTEK_LWIP_TIMEOUT_FIX.patch`.

**Nota GitHub:** questa pagina è un checkpoint; i sorgenti completi CP-16 **non risultano sincronizzati integralmente nel repository**. La pubblicazione del documento non attiva la compilazione nativa del CP-16.

## Prossima priorità
SDK vendor KM0/KM4 reale, integrazione su toolchain ufficiale e build/diagnostica; poi storage persistente e API autenticate, infine test hardware con recovery.