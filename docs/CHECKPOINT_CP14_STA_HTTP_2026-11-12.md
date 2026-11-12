# GW006 IR Universal — Checkpoint CP-14 (12 novembre 2026)

**Target:** Tuya RSH-GW006, WBRG1/RTL8721CSM. **OpenBeken 1.18.321 rimane intatto.** Nessun firmware CP-14 installabile o verificato su hardware; **NON FLASHARE**.

## Verificato
- Struttura 96 tasti: TV36, Clima14, Decoder32, Speaker14; fino a cinque pacchetti IR/tasto. Memoria flash completa sulla GW006 non ancora integrata.
- Protocollo IR/TCP 42424 HMAC-SHA256 con anti-replay: test su host già passati.
- CP-13: dashboard REV18 compressa gzip (~5,1 KB da ~13,2 KB) incorporata come risorsa C; gestore HTTP di sola lettura (GET/HEAD), rifiuta POST e comandi IR anonimi.
- CP-14: aggiunto `firmware/realtek/gwir_dashboard_sta_server.c/.h`: listener POSIX/lwIP candidato con bind a IPv4 selezionato, callback di controllo STA, socket timeout, gestione di una richiesta per connessione e chiusura socket.
- Test CP-14 rieseguiti il 12/11/2026 su Linux con `make test-dashboard-sta-host test-dashboard-live-tcp test-dashboard-sanitized`: **PASS** (TCP GET reale, frammentazione, POST rifiutato, gzip/CSP, ASan/UBSan). Archivio ZIP CP-14 verificato con `unzip -t` (nessun errore).
- Sorgenti locali chat: `GW006_IR_UNIVERSAL_CP14_STA_HTTP_SOURCE.zip`; SHA-256 `00fa45af7fc1be8d668c3925eb89af1eeb27acebbfb3cfa01c5fc827ebb388a5`. **Il repository non contiene automaticamente il contenuto integrale di questo ZIP.**

## Mancano (bloccanti)
1. Aggancio reale a driver Wi-Fi STA Realtek, task FreeRTOS e stato della rete. **Bind IP STA non isola automaticamente APConfig:** implementare controllo di interfaccia e accesso effettivo prima di esporre il server.
2. Compilazione *nativa completa* SDK KM0/KM4 degli ultimi sorgenti. La REV16 nativa riuscita non prova CP-14; REV21 GitHub era ancora in coda: https://github.com/lorenzo2979/loreme/actions/runs/43095247514 .
3. Memoria persistente dei codici 96 x 5 sul chip con test capienza, coerenza e perdita di alimentazione; sincronizzazione app/web.
4. Endpoint IR web **autenticati**, difesa CSRF/replay. Dashboard attuale è una UI in anteprima, senza controllo reale IR.
5. Prova hardware PA25 TX/PA26 RX e isolamento del LED rosso su PA25. Stadio BC337 alimentato a 5V: resistenza 220 Ω sul LED, 1 kΩ tra GPIO e base, GND comune, nessun ritorno 5V sul GPIO; verificare pinout.
6. Backup completo flash originale e recovery hardware collaudato prima del flash; OpenBeken preservato.

## CI / prossima ripresa
Core tests GitHub riusciti: https://github.com/lorenzo2979/loreme/actions/runs/43111048891 . Non equivale a collaudo WBRG1.

**Prossimo passo:** integrare Wi-Fi STA/FreeRTOS/HTTP nel progetto SDK nativo e ottenere una build reale; poi storage e API web protette, infine hardware e recovery.