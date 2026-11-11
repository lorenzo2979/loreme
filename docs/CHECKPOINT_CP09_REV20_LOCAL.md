# CP-09 — GW006 IR Universal REV20
**Data:** 11 novembre 2026  
**Stato:** sorgenti REV20 locali verificati; **NON** firmware flashabile.

## Continuità
- Tuya GW006 / WBRG1 RTL8721CSM. OpenBeken 1.18.321 ancora funzionante dichiarato, non modificato.
- Pin candidati IR: TX PB31, RX PA26; pinmux e driver elettrici NON provati sulla scheda.
- 96 tasti: TV36, Clima14, Decoder32, Speaker14; fino a 5 pacchetti IR per tasto.
- REV18 dashboard HTML è offline, senza vera gestione IR/network. APK Android GW006 completo non presente.
- TCP IR 42424 + HMAC-SHA256 è protocollo software, non ancora un listener reale sul dispositivo.
- APConfig: SSID GW006-IR-SETUP, AP volutamente aperto per 5 minuti dopo pressione pulsante 5 s; DHCP e HTTP server reali non presenti.

## Nuovo REV20
- Moduli `firmware/core/gwir_apconfig_http_stream.c/.h` aggiunti **localmente**: ricezione HTTP/1.1 frammentata, nessuna allocazione dinamica, max 1.536 byte (header 1.024, body 512).
- L'assembler accetta soltanto una richiesta, rifiuta pipelining nello stesso segmento, valori Content-Length duplicati, request overflow, timeout e accesso fuori APConfig.
- Richiede attestazione affidabile **dal trasporto** della rete AP, non fiducia nell'header Host o nell'IP del mittente.
- Idle timeout 2 s, totale 10 s; risposte 204/400/403/408/413/503 con chiusura della connessione, zero del buffer credenziali.
- Si appoggia al parser REV19 e al contratto `stage / verify / commit / rollback`. Salvataggio non volatile e associazione STA **non** implementati sul Realtek.
- Test C aggiunti e Makefile/script SDK mock aggiornati per **26 oggetti**.

## Esito test locali
- PASS `make test test-sdk-integration test-web-assets test-kotlin arm-smoke test-http-sanitized test-stream-sanitized`.
- 3.000 sequenze deterministiche di frammentazione TCP casuale, oltre a confini e timeout, revoca AP, pipeline, rollback, write failure.
- REV19: 30.000 campioni HTTP malformati sotto ASan/UBSan; REV20 stream test sotto ASan/UBSan.
- Compilazione ARM Cortex-M23/M33 compile-only con stub, **non** build SDK completa, test Wi-Fi o prova su hardware.

## Artefatto
- Archivio nella conversazione: `GW006_IR_UNIVERSAL_REV20_SOURCE_TESTED.zip` (142 file; SHA-256 `eba6adadebc4fc0d37606235e016fb4d3a174f3746c985fd2a268a2dbb824af6`).
- Log: `GW006_REV20_TEST_RESULTS.log`; checkpoint integrale: `CHECKPOINT_GW006_REV20_CP09.md`.
- **Il repository GitHub non contiene ancora necessariamente tutti i sorgenti REV20**: questo documento registra solo il checkpoint.

## Prossimi interventi non risolti
1. Implementare adapter vero nel vendor SDK: AP-only listener con verifica effettiva netif, DHCP, IPv4 AP, chiusura e timer 5 min.
2. Backend STA con verifica reale e persistenza atomica/power loss recovery.
3. Build nativa RTL8721CSM KM0/KM4, gestione memoria 96x5, GPIO IR e prove reali.
4. Backup flash completo + recovery validato prima di qualsiasi nuova installazione.

**Non flashare questo pacchetto. Proteggere OpenBeken 1.18.321.**
