# GW006 IR Universal — Checkpoint CP-18 (12 novembre 2026)

**SOURCE-ONLY: NON FLASHARE.** OpenBeken 1.18.321 deve restare intatto.

## Nuovo archivio flash NOR: simulato, non collegato a hardware
- firmware/core/gwir_flash_journal.c/.h: journal append-only per 96 record GWCD (5 frame ciascuno). Due settori manifest da 4 KiB, CRC32, generazioni e commit marker programmato alla fine.
- Prevalidazione dati GWCD, scrittura e rilettura prima del commit. In caso di interruzione il vecchio record resta recuperabile; i settori scritti parzialmente vengono saltati.
- API su callback astratte read/program/erase_sector; nessun accesso reale alla flash WBRG1. Solo su una partizione DEDICATA da identificare.
- Senza garbage collection, gli aggiornamenti consumano progressivamente lo spazio. Nessuna promessa di cicli di scrittura illimitati.

## Verificato su computer (PASS)
- Scrittura e lettura di 96 tasti al massimo (5 × 2048 impulsi), recovery dopo reboot, cancellazione, CRC corrotto, no-space, manifest danneggiati.
- Interruzione virtuale di alimentazione dopo ogni byte dell'aggiornamento del record: al riavvio vecchio o nuovo record sempre integro.
- Sanitizzatori ASan/UBSan, compilazione preliminare ARM Cortex-M23/M33 con header mock.
- Regressioni C, Kotlin/JVM, autenticazione, dashboard HTTP/lwIP, 35 oggetti SDK mock con rollback. Test completi terminati con exit 0.

## Budget minimo teorico
- Dimensione record massimo 41.040 B; 11 settori da 4 KiB = 45.056 B per tasto.
- 96 × 45.056 B + 2 × 4.096 B = **4.333.568 byte** dedicati, SENZA firmware/OTA e SENZA spazio per riscritture.
- Verificare la vera mappa delle partizioni del WBRG1 prima di collegare qualsiasi driver flash.

## Artifacts
- GW006_IR_UNIVERSAL_CP18_NOR_JOURNAL_SOURCE.zip, 197 file, SHA256 00f56ca2f2e5b23ad4739909f5d8f17cfa3eb817c82dedb736cd1512b4585ba7
- GW006_CHECKPOINT_CP18_2026-11-12.md, GW006_CP18_COMPLETE_TESTS.log, GW006_CP18_CHANGES_FROM_CP17.patch

**GitHub non contiene ancora tutti i sorgenti CP-18: questo file e' soltanto il checkpoint.** Non esiste una build nativa Realtek KM0/KM4 CP-18 verificata, né un firmware flashabile.

Prossimo: sincronizzazione sorgenti; build SDK nativa; mappa flash, adapter hardware e compattazione sicura; API web IR autenticate; test su hardware con backup/recovery OpenBeken.