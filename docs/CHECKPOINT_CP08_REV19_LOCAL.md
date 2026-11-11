# CHECKPOINT CP-08 — GW006 IR Universal REV19 (sorgenti locali)
**Data:** 11 novembre 2026  
**Stato:** PASS dei test software locali; NON pronto per il flash.

## Base verificata
- Hardware: Tuya RSH-GW006, modulo WBRG1 / Realtek RTL8721CSM.
- Firmware funzionante dichiarato sulla scheda: **OpenBeken 1.18.321**, non modificato.
- Pin IR **candidati**: TX PB31, RX PA26 (non collaudati elettricamente in funzionamento IR).
- 96 tasti previsti: TV36, Clima14, Decoder32, Speaker14, fino a cinque pacchetti per tasto.
- Dashboard REV18: anteprima HTML offline, non collegata ad alcun dispositivo.
- Protocollo IR LAN: TCP 42424, GWAS HMAC-SHA256; test software, non servizio Realtek operativo.
- APConfig desiderato: pulsante 5 secondi, AP aperto per 5 minuti, SSID `GW006-IR-SETUP`, senza login/password/token nella sola finestra temporanea di configurazione.

## Cosa è stato sviluppato nella REV19
Nel pacchetto di sorgenti locale `GW006_IR_UNIVERSAL_REV19_SOURCE_TESTED.zip` è stato creato:
- `firmware/core/gwir_apconfig_http.c` e relativo header: parser HTTP/1.1 a memoria limitata (fino a 1024 B di header e 512 B di form body) che accetta soltanto `POST /api/provision` per l'host `192.168.4.1`.
- Rigetti di Transfer-Encoding, upgrade, duplicazioni di Content-Length/Host, body di dimensione incoerente, HTTP pipelining, header/form malformati.
- Verifica obbligatoria che APConfig sia attivo e che il socket provenga dall'interfaccia AP **attestata dal trasporto**; il codice usa il backend astratto stage/verify/commit/rollback del profilo STA.
- Nuovi test C funzionali e stress deterministico di **30.000 input HTTP** sotto AddressSanitizer/UndefinedBehaviorSanitizer.
- Integrazione del nuovo modulo nel progetto compilabile con mock SDK; **25 oggetti mock** e compilazione ARM compile-only Cortex-M23/M33.

Tutti i seguenti controlli **locali** risultano superati: `make test test-sdk-integration test-web-assets test-kotlin arm-smoke test-http-sanitized`.

**Il codice REV19 integrale NON è ancora pubblicato nel repository GitHub**: il repository principale osservato ha README REV16. Il file ZIP locale (136 file, SHA-256 `197f63e9909ce6ea7479156abe89a3184af10f8a938a5fd782c9bbb5dc6d6895`) è la copia fonte di questa milestone, da conservare o importare come revisione verificabile prima di eseguire CI nativa.

## Limiti non risolti
- Non esistono ancora listener HTTP/DHCP reali su AP, driver di rete STA completo, persistenza atomica delle credenziali e profili Wi-Fi, o listener TCP autenticato operativo sulla GW006.
- Nessuna compilazione completa in SDK Realtek per REV19; il mock e l'ARM compile-only non rappresentano la validazione nativa.
- Pinmux IR RX/TX, tensioni elettriche, portante 38 kHz, boot/partizioni flash, backup binario e recovery non verificati in questa revisione.
- L'APConfig aperto è accessibile ai dispositivi vicini durante la finestra di attivazione: il futuro socket dovrà vincolarsi **solo** all'interfaccia AP, chiudere la sessione entro cinque minuti e non esporre comandi IR.
- **NON flashare** sulla GW006 e preservare OpenBeken 1.18.321.

## Prossimi interventi
1. Creare listener AP-only, gestione HTTP frammentato e timeout, server DHCP e verifica dell'interfaccia accettata.
2. Integrare il backend STA con persistenza atomica e verifica reale dell'associazione, senza perdere le vecchie credenziali.
3. Sincronizzare il pacchetto sorgente REV19 con GitHub ed eseguire la build nativa KM0/KM4 su SDK Realtek.
4. Collaudare pinmux/IR e memoria 96×5 su hardware, poi qualificare backup/boot/recovery prima di valutare il flash.
