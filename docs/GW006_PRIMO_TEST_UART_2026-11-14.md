# GW006 IR Universal — prima prova sul banco (14 novembre 2026)

**Versione:** test hardware preliminare, NON firmware IR Universal definitivo.
**SoC reale:** WBRG1 / RTL8721CSM. **Compilazione:** Realtek AmebaD Arduino 3.1.9 con scheda di riferimento RTL8721DM (AW-CU488). Non è stata ancora verificata l'accensione di questa immagine sulla GW006.

## Cosa contiene

Il pacchetto GitHub Actions `gw006-realtek-km0-km4-experimental-boot-inspection` conserva:
- `km0_boot_all.bin`
- `km4_boot_all.bin`
- `km0_km4_image2.bin`
- `MANIFEST.json` con SHA-256/numero di byte e commit
- `READ_ME_FIRST.txt`

Workflow: `.github/workflows/gw006-ameba-arduino-probe.yml`.

I tre file sono un insieme di immagini formato Realtek. **Non si deve presumere che siano automaticamente compatibili con il bootloader, la mappa Flash o la calibrazione della GW006** solo perché il linker ARM ha prodotto un ELF valido.

## Prima dell'installazione

- Sulla tua GW006 OpenBeken non funziona più e non vuoi recuperare i codici IR; questo non elimina il rischio di sovrascrivere parti necessarie all'avvio.
- Occorre poter raggiungere la **UART Realtek** con adattatore USB-TTL **a logica 3,3 V**, massa condivisa e TX/RX incrociati. Non collegare l'uscita 5 V dell'adattatore ai GPIO del modulo.
- Controllare il cablaggio del ricevitore IR **PA26** (3,3 V) e verificare l'effettiva rimozione del LED rosso sulla rete **PA25** prima della prova TX. PA25 è solo un segnale per uno stadio IR a transistor, non una sorgente diretta per LED IR ad alta corrente.
- Confermare la modalità UART download/recovery e l'identità della Flash prima di qualunque cancellazione. Non suggeriamo comandi di cancellazione indiscriminata o indirizzi ricavati dai dump pubblici.
- L'immagine **non attiva** la scrittura dell'archivio IR a 96 tasti finché non viene riservata una partizione adatta e verificato l'avvio.

## Diagnostica UART dopo un avvio riuscito

Aprire un terminale seriale a **115200 baud**, 8N1, e inviare una riga terminata con INVIO:

| Comando | Risultato atteso |
|---|---|
| `HELP` | Elenco dei comandi realmente disponibili |
| `STATUS` | `GW006_RTL8721CSM_TEST: RTOS_BOOTED` e stato runtime |
| `FLASHID` | JEDEC letto dalla Flash senza scrivere. Il byte capacità `17` rappresenta 8 MiB, se ID valido |
| `PINS` | PA25 TX, PA26 RX, PB22 LED blu, PB23 tasto |
| `WIFI` | Chiede SSID e password sulla UART fisica e prova a collegarsi in modalità STA; **non salva** le credenziali. Non viene aperto un server di configurazione senza autenticazione |
| `WIFISTATUS` | Stato e IP se connesso |
| `TXTEST-PA25-LED-REMOVED` | Singolo breve segnale 38 kHz su PA25, **solo dopo aver rimosso il LED rosso**, verificabile con ricevitore IR/oscilloscopio |
| `RXTEST-PA26` | Finestra di 4 secondi per provare un telecomando **NEC**, solo come test del ricevitore. Un telecomando non-NEC può dare timeout pur avendo il ricevitore funzionante |

I test TX/RX usano la libreria `IRDevice` ufficiale Realtek e sono separati dall'acquisizione universale del firmware GWIR. Non attestano che 2048 impulsi, cinque pacchetti e persistenza funzionino già sulla scheda.

## Ancora da completare per il firmware definitivo

1. Adattamento bootloader/immagini e layout **RTL8721CSM** certificato sulla GW006, con prova reale di avvio.
2. Provisioning persistente Wi-Fi, chiave HMAC univoca e nonce fresco verificato; l'implementazione corrente rifiuta correttamente l'attivazione normale se mancano.
3. Partizione NOR IR dedicata: la proposta `0x200000..0x6FFFFF` richiede **progettazione del nuovo layout completo**, non è libera in tutte le immagini pubbliche del vecchio firmware. Il caso peggiore 96 × 5 con riscrittura atomica richiede 4.378.624 byte.
4. Prova reale di RX/TX universale, salvataggio persistente e recupero dopo spegnimento; app Android completa ancora da realizzare (sono disponibili librerie Kotlin, non un'APK finale).

**Questo documento distingue una build tecnica riuscita da un firmware pronto per la vita quotidiana.**
