# CP-13 — Dashboard REV18 compressa e risponditore HTTP C

Data: **11 novembre 2026**. **PROTOTIPO SOFTWARE, NON FLASHARE.**

## Risultato verificato
- Dalla base locale CP-12 è stato creato un archivio **CP-13** con 164 file e SHA-256 `e4a40f17e8e572c8ea8b4cadfc4a1881a6d5b80b3232badb764bd1092b7a40cb` (consegnato nella chat come `GW006_IR_UNIVERSAL_CP13_DASHBOARD_SOURCE.zip`).
- Dashboard REV18 con TV36, Clima14, Decoder32, Speaker14 = 96 pulsanti, tema scuro viola, menu a pressione lunga, schermata Wi-Fi/MQTT/DuckDNS, **solo in anteprima** e senza azioni reali.
- `scripts/embed_dashboard.py` produce un asset immutabile gzip C di **5.094 byte** a partire dall'HTML originale di 13.193 byte, con CSP SHA-256 verificata per JS/CSS inline.
- `firmware/core/gwir_dashboard_http.c/.h` restituisce **solo** GET /, GET /index.html e HEAD; rifiuta richieste con body, header doppi, metodi non consentiti e pipelining; risposta `Connection: close`, `Cache-Control: no-store`, CSP `connect-src 'none'`, X-Frame-Options DENY. Max request 1.024 B; body emesso in segmenti da 384 B; niente allocazioni HTML decompressa in RAM.
- Test C: 25 frammentazioni, partial writes, richieste non autorizzate, gzip, CSP, socketpair; test **HTTP IPv4 host reali** con browser-style GET su 127.0.0.1 che decomprime il body e confronta l'HTML con la sorgente.
- PASS suite regressione core, Kotlin, SDK mock (**30 sorgenti**), ARM compile-only, sanitizer ASan/UBSan, APConfig HTTP, GWAS auth, TCP live. Registrazione locale `GW006_CP13_FULL_TESTS.log` e `GW006_CP13_LIVE_HTTP.log`.

## Confini del test
**Il modulo C non è ancora un server realmente in ascolto sulla GW006.** Non è stato scritto/avviato il listener lwIP, il task FreeRTOS o il bootstrap Wi-Fi STA sul vendor SDK; nessun firmware CP-13 native KM0/KM4 certificato. La REV16 era compilata in SDK; run REV21 era ancora in coda al controllo precedente. Non affermare che l'interfaccia controlla hardware.

**La dashboard su chip senza Android richiede ancora storage persistente** dei 96 codici, API IR HTTP autenticate e anti-CSRF, prova effettiva su WBRG1. Non esporre controlli IR tramite endpoint anonimi o sulla APConfig aperta.

## Pin / ripristino
Preferenza candidata **TX PA25 / RX PA26**; PA25 è fisicamente collegato al LED rosso che va isolato e verificato prima di un collegamento; BC337 5V richiede resistenza di base 1 kΩ e massa comune, con prevenzione ritorno 5V su GPIO. Non c'è prova TX/RX su hardware. **OpenBeken 1.18.321 deve restare invariato** fino al backup integrale e a un recovery collaudato.

## Prossimo vero traguardo
1. Build SDK nativa CP-13.
2. Task HTTP lwIP read-only **solo STA LAN**, avvio/disattivazione secondo Wi-Fi e verifica col browser sulla scheda.
3. Memoria flash persistente con test di capienza e power-loss, quindi API web autenticata per telecomandi.
4. Collaudo IR fisico e recovery prima del flash.

Questo checkpoint registra **codice host verificato**, non un firmware utilizzabile.