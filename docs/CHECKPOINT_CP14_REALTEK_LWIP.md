# Checkpoint CP-14 — GW006 IR Universal — 11 novembre 2026

**Stato: modulo dashboard lwIP/FreeRTOS sviluppato e testato in simulazione, NON firmware installabile.**

## Avanzamento verificato
- Base CP-13: dashboard REV18 (TV36, Clima14, Decoder32, Speaker14 = 96 pulsanti), asset gzip in C **5.094 B** da HTML 13.193 B, server HTTP soltanto GET/HEAD.
- Aggiunti `firmware/sdk/gwir_sdk_dashboard_lwip.c/.h`: adapter progettato per le API ufficiali AmebaD lwIP e FreeRTOS. Task HTTP sulla porta **8080**; bind rigoroso **solo all'IPv4 della STA**, mai `INADDR_ANY`; rete APConfig attiva blocca il servizio; blocco di indirizzi non privati, verifica continua di stato Wi-Fi/IP, rifiuto di richieste durante il cambio IP.
- `firmware/core/gwir_dashboard_net_gate.c/.h`: controllo di indirizzi RFC1918 e stato di rete. No modifiche all'IR e nessuna nuova API web di comando.
- `scripts/integrate_amebad_sdk.py` include due nuovi moduli nella configurazione sorgenti SDK. Il codice chiama il bootstrap da un punto esistente, ma **`GWIR_BOARD_HTTP_ENABLE=0` è il valore di default**: nessun listener viene avviato nelle build ordinarie.
- Test SDK emulato: **32 oggetti** e integrazione applica/rimuovi idempotente.
- Test abilitazione con stub FreeRTOS/lwIP: bind a IP STA esatto, gestione TCP frammentata e scritture parziali, rifiuto client pubblici, AP acceso, IP cambiato, netif giù, inibizione metà trasferimento. **PASS**.
- Regressione locale completa PASS (core IR, GWAS HMAC e anti-replay, Wi-Fi/APConfig, Android/Kotlin, HTML gzip, socket POSIX reali, sanitizzatori di memoria, compile-only ARM Cortex-M23/M33). **Esito finale 0**.
- Archivio sorgenti nella conversazione: `GW006_IR_UNIVERSAL_CP14_LWIP_SOURCE_ONLY.zip` — **178 file**, SHA-256 **`463f825f08be836286609d8178f8a1451fb009918316090b5f02aa381fa82995`**. In particolare contiene `docs/CP14_LWIP_REALTEK_DASHBOARD.md` e `SOURCE_SHA256SUMS_CP14.txt`.
- Log locale: `GW006_CP14_FINAL_TESTS.log` e `GW006_CP14_VENDOR_ASAN.log`.

## Cosa NON è stato verificato
- Nessuna nuova build **nativa KM0/KM4 CP-14** dell'SDK completo. La vecchia REV16 aveva superato SDK-native; REV21 GitHub Actions run [43095247514](https://github.com/lorenzo2979/loreme/actions/runs/43095247514) continua a risultare `queued`, senza esito.
- Nessun server HTTP è stato avviato sulla **GW006 reale**, nessuna prova radio o FreeRTOS sui suoi limiti RAM/flash.
- La dashboard serve **solo pagina statica**, senza comandi IR web; per essere autonoma occorrono gestione dei 96 codici su flash con capienza/recovery e API IR web autenticate.
- Scelta candidata pin **PA25 TX / PA26 RX**. PA25 è connesso al LED rosso originale, da isolare dopo verifica circuitale. Circuito esterno BC337 a 5 V con resistenza su LED di 220 ohm, **resistenza di base 1k ohm da aggiungere/verificare**, massa comune e nessun 5 V diretto sul GPIO. Nessun pin è stato attivato.
- OpenBeken **1.18.321 preservato**. Nessun flash; backup integrale e percorso di recupero non ancora convalidati.

## Prossima priorità
1. Portare il codice sorgente CP-14 nella pipeline Realtek SDK e ottenere **KM0/KM4 native PASS** su questo codice, non sul precedente REV16.
2. Verificare avvio della dashboard read-only su WBRG1 con Wi-Fi reale, soltanto **dopo** backup/recovery del dispositivo.
3. Implementare archivio codici sul chip, API autenticate browser e motore IR fisico. Non dichiarare il firmware pronto prima di questi test.

**Nota:** questa pagina GitHub documenta CP-14. I sorgenti CP-14 completi sono nell'archivio della conversazione; non sono ancora sincronizzati integralmente come file nel ramo principale GitHub.
