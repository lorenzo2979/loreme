# GW006 / WBRG1 — confronto di quattro dump pubblici della flash (14/11/2026)

**STATO: EVIDENZA DI RIFERIMENTO, NON MAPPA PARTIZIONI, NON AUTORIZZAZIONE AL FLASH.**

## Esecuzione verificata

- Workflow read-only: `.github/workflows/gw006-public-reference-sector-audit.yml`
- Build: https://github.com/lorenzo2979/loreme/actions/runs/43564405709
- Origine: `openshwprojects/FlashDumps`, cartella `IoT/RTL8721CSM`
- 4 immagini complete da 8.388.608 byte ciascuna, identità verificata con SHA1 dei Git blob originali.
- Etichette: Tuya Gateway 1.7.2, GW018-DM 1.6.2, Maginon WT-2G 1.7.2, TYGWZB-01N 1.8.2.
- Per ragioni di riservatezza, il workflow non allega né salva i dump originali come artifact: potrebbero contenere credenziali o dati personali di altri dispositivi.

## Confronto settore per settore (4.096 byte)

| Misura | Risultato |
|---|---:|
| Settori per dump | 2048 |
| Settori identici in tutte e quattro le immagini | 928 |
| Settori diversi in almeno una immagine | 1120 |
| Settori completamente `FF` in tutte e quattro | 885 |
| Byte complessivi di quei settori `FF` | 3.624.960 (3,46 MiB) |
| Maggiore intervallo contiguo identico / completamente `FF` | 0x184000, 1.040.384 byte |

**NON** dedurre che gli 885 settori comuni `FF` siano liberi, utilizzabili o riservabili.
Sono distribuiti in regioni separate e possono appartenere a slot OTA,
bootloader, FTL, dati di calibrazione o altre strutture che oggi non contengono
dati ma che il firmware può usare in futuro.

## Controllo specifico della proposta di partizione IR di 5 MiB

Il file `firmware/realtek/gwir_wbrg1_flash_port.h` proponeva, ma NON abilitava,
la zona **0x200000..0x6FFFFF** come IR journal. Contiene dati già non-`FF`
nelle quattro immagini di riferimento:

| Riferimento | Settori da 4 KB NON cancellati nella zona proposta |
|---|---:|
| Tuya Gateway 1.7.2 | 606 |
| Tuya GW018-DM 1.6.2 | 642 |
| Tuya Maginon 1.7.2 | 646 |
| Tuya TYGWZB-01N 1.8.2 | 646 |

**Conclusione:** la proposta di regione 5 MiB NON può essere trasformata
automaticamente in partizione di scrittura; la semplice modifica di
`GWIR_WBRG1_STORAGE_APPROVED` a 1 non è un'autorizzazione valida. Preservare
lo stato di compilazione fail-closed e non alterare OpenBeken 1.18.321.

## Vincoli per il firmware finale

La dimensione peggiore del journal GWCD v1 con 96 tasti, fino a 5 frame per
tasto, più uno spazio di sostituzione atomica, è **4.378.624 byte**.
Non è stata individuata alcuna partizione verificata per GW006 di tale
dimensione. Il confronto dei dump di terzi non dimostra la capienza disponibile
sulla scheda dell'utente.

Per realizzare una release installabile, occorrono mappa effettiva e backup
non invasivo dell'esemplare, piano di ripristino provato, verifica del boot,
pinmux IR, della RAM, provisioning del segreto/nonce e collaudo della
memoria 96×5. Le tre immagini KM0/KM4 prodotte in precedenza sono prove
di compilazione, **NON flashabili**.

## Note hardware da documentazione Tuya

- `PB31` è etichettato `AOUTP_L`, uscita audio analogica: non trattarlo
  come pin IR TX validato.
- `PA25` è GPIO/PWM e nella GW006 è stato identificato come possibile
  conflitto con LED rosso; va verificato elettricamente.
- `PA26` è identificato nel datasheet come `REQ` PTA, non come attestazione
  del collegamento effettivo del ricevitore IR sulla GW006.

Riferimenti:
- https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz
- https://www.elektroda.com/rtvforum/topic4127578.html
- https://github.com/openshwprojects/FlashDumps/tree/main/IoT/RTL8721CSM

**Non ripubblicare chiavi, configurazioni Wi-Fi o dump integrali di altri utenti.**
