# GW006 — audit in sola lettura della Flash ricomposta (14 novembre 2026)

**Stato: SOLO ANALISI, NON FLASHARE.** Nessuna scrittura su hardware, nessun dump privato caricato su GitHub.

## Input della scheda personale (file tenuti privati)
- Primo segmento: `RTL8720D_QIO_obk229EED.bin`, 2.097.152 byte, offset `0x000000..0x1FFFFF`.
- Secondo segmento: `200000-600000`, 6.291.456 byte, offset `0x200000..0x7FFFFF`.
- Immagine ricomposta: `GW006_RIPRISTINO_FLASH_ORIGINALE_8MiB.bin`, 8.388.608 byte.
- Verifica locale byte per byte: **primo_segmento + secondo_segmento == immagine_ricomposta: PASS**.
- SHA256 del ricomposto: `4f1731e44b83e2b473d4e67b9ab10fa2b9fd4c61c887124abd99c2feeba753c7`.
- NOTA: il ricomposto è l'unione di due regioni, **non** costituisce due acquisizioni fisiche integrali indipendenti; non certifica recuperabilità.

## Occupazione della NOR da 8 MiB (settori 4 KiB)
- Totali: **2.048 settori**.
- Tutti `0xFF`: **559 settori** (2.289.664 byte); non per forza disponibili.
- Con almeno un byte non-`0xFF`: **1.489 settori**.
- Intervallo `0x000000..0x1FFFFF`: 136 settori FF / 376 non-FF.
- Intervallo `0x200000..0x6FFFFF`: 423 settori FF / 857 non-FF.
- Intervallo `0x700000..0x7FFFFF`: 0 settori FF / 256 non-FF.
- Maggiore intervallo consecutivo tutto FF: `0x5E3000..0x6D6FFF`, **999.424 byte (976 KiB)**.

## Verdetto
Il journal GWIR per 96 tasti con fino a 5 pacchetti richiede **4.378.624 byte / 1.069 settori** nel caso peggiore. I soli settori attualmente cancellati sono 559, discontinui, e il maggiore blocco consecutivo è 976 KiB. **Non esiste nell'immagine corrente un intervallo cancellato adeguato**.

Questo non rende impossibile il firmware da 8 MiB: richiede **nuova partizione e riorganizzazione degli spazi in sede di sostituzione firmware**, solo dopo identificazione e preservazione di bootloader, calibrazione e config, seconda lettura indipendente di backup, prova reale di recovery, e validazione KM0/KM4. Non usare aree che sembrano `FF` senza mappa boot/OTA certificata.

`GWIR_WBRG1_STORAGE_APPROVED=0` resta obbligatorio. `main` e OpenBeken 1.18.321 non vanno modificati.

**Audit locale eseguito effettivamente** su file di libreria; i dump privati non sono inclusi nel repository.