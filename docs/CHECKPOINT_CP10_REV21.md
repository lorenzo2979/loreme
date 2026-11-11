# GW006 IR Universal — CP-10 (11 novembre 2026)

## Progresso effettivamente verificato

1. **SDK nativo già compilato con GW006 REV16.** GitHub Actions run [43073357001](https://github.com/lorenzo2979/loreme/actions/runs/43073357001), esito `success`: clonazione SDK Realtek AmebaD, integrazione GW006, build KM0 e build KM4 riuscite. Questo è test SDK e **non collaudo fisico** della GW006, né compilazione della REV21.
2. **REV21 locale:** completato `tests/test_apconfig_tcp_transport.c`, precedentemente assente e causa di errore `No rule to make target ...`; si verifica il nuovo codice `gwir_apconfig_tcp_transport.c/.h` per una sola connessione APConfig, con letture frammentate, scritture parziali, timeout, revoca della rete AP e rollback del profilo Wi-Fi.
3. **Regressione locale:** PASS `make test test-sdk-integration test-web-assets test-kotlin arm-smoke test-http-sanitized test-stream-sanitized test-tcp-transport-sanitized`. Host/GCC, SDK mock, Kotlin, clang ARM compile-only e ASan/UBSan; NON SDK-native REV21.
4. **Sorgenti REV21 non ancora sincronizzati integralmente su GitHub:** archivio locale `GW006_IR_UNIVERSAL_REV21_SOURCE_TESTED_OFFLINE.zip` (146 file; SHA256 `160b3788f0afa3125305c63fd06234384e77b813cd227dca5f3b95fca7521855`). Log locale: `GW006_REV21_VERIFIED_LOCAL_2026-11-11.log` SHA256 `8280dfcd0ab05f06a6902c480aac4111cdd23056702130bc8df9f6a16a498cd5`.

## Stato di progetto

| Funzione | Stato |
|---|---|
| GW006 / WBRG1 RTL8721CSM, OpenBeken 1.18.321 | Firmware funzionante segnalato; non modificato |
| IR TX/RX | Portable core sviluppato, pinmux/reale scheda ancora da verificare |
| TX PB31 / RX PA26 | Pin candidati, NON autorizzati per flash/collegamenti definitivi |
| 96 tasti, TV36/Clima14/Decoder32/Speaker14, max 5 pacchetti | Componenti e test offline, storage persistente board mancante |
| HMAC-SHA256 e anti-replay | Host e compatibilità software testati; listener LAN su board mancante |
| APConfig OPEN 5min via pulsante 5s | Logica/offline sviluppata; vera integrazione AP/DHCP/socket non completata |
| Gestione Wi-Fi STA | Parsing e contratti stage/verify/commit/rollback; persistenza reale mancante |
| Dashboard web | Anteprima offline; non connessa al firmware |
| Android | Componenti JVM, non APK completo GW006 |
| Compilazione SDK nativa | **REV16 riuscita** in CI GitHub; REV21 ancora da portare e compilare |

## Percorso più rapido verso hardware vero

1. Trasferire REV21 completa in GitHub mantenendo stato verificabile e rieseguire build SDK KM0/KM4 sul codice aggiornato.
2. Implementare nel vendor SDK la socket AP-only con attesta netif affidabile, DHCP, HTTP reali, prova di Wi-Fi STA e salvataggio atomico.
3. Collaudare con strumenti i pin IR PB31 TX/PA26 RX, portante, acquisizione RX e replay; la documentazione Realtek/esempi ha discrepanze nella mappatura PB31.
4. Collegare HMAC/TCP 42424 a TX/RX e memorizzazione non volatile dei 96×5 codici; completare collegamenti dashboard/app.
5. Backup completo e prova di recovery **prima** di qualsiasi flash. Non sostituire l'OpenBeken funzionante senza un percorso di ripristino.

**Non esiste ancora un firmware GW006 REV21 qualificato da flashare.**
