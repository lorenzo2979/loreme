# GW006 CP43 — backup integrale e mappa Flash, procedura SOLO LETTURA
14/11/2026 · RSH-GW006 / WBRG1 / RTL8721CSM

> **Nessun backup della scheda personale risulta disponibile nel repository o nei file esaminati. Stato: BLOCKED / NO FLASH.**
> Tutti i riferimenti ad altre GW006/WBRG1, anche con 8 MiB e identico chip, sono dati di TERZI e NON verificano il layout di questa scheda.

## Obiettivo e programma che è stato realmente realizzato
`scripts/verify_gw006_dual_backup_readonly.py` legge esclusivamente due FILE locali, ciascuno esattamente di 8.388.608 byte (8 MiB), ottenuti da due operazioni UART separate sul medesimo esemplare.
Confronta *tutti* i byte (settori da 4096), calcola SHA256, rifiuta troncamenti, divergenze, dati uniformi, percorsi coincidenti e hardlink.
Conta i settori tutti `FF` e quelli non `FF` nella vecchia proposta 0x200000..0x6FFFFF.
Il rapporto JSON NON contiene credenziali, nomi file, contenuto dei settori o chiavi.

**Il programma non può garantire che i due file provengano da due vere letture indipendenti.**
Un file copiato due volte potrebbe coincidere ugualmente. Conservare il log di due letture, data e collegamenti come prova separata.
La parola 'matching' NON è sinonimo di bootloader salvato, mappa certificata o recupero testato.
`flash_allowed: false` rimane sempre presente, incluso quando il confronto dei byte riesce.

## Cosa serve lato utente per poter completare l'audit
- Un computer con Python 3 e un adattatore USB↔UART **a livello logico 3,3 V** verificato, con collegamento ad alimentazione corretta della GW006; **mai 5 V su GPIO** e mai lasciare dispositivi connessi alla rete elettrica durante interventi.
- Identificazione della revisione del PCB e del connettore di debug sulla *tua* scheda. Non collegare fili seguendo alla cieca foto di altri modelli. Se non hai esperienza di elettronica, richiedere assistenza per la sola lettura.
- Uno strumento adatto al **WBRG1/AmebaD**, con comando che **legge** e non scrive, verificato prima dell'uso. Non scegliere funzioni tipo "Backup and flash new": alcuni strumenti proseguono direttamente con cancellazione e scrittura.
- Due letture complete *distinte* e relativi log, senza riutilizzare/duplicare il file della prima lettura.

### Contesto tecnico e comando di lettura della comunità (NON eseguito sul tuo hardware)
La guida specifica RSH-GW006 su Elektroda:
https://www.elektroda.com/rtvforum/topic4127578.html
indica un tool `rtltool.py` con operazione `rf` (read flash). Nel testo del forum è presente
`0x8000000`, ma **è un valore errato se l'intento è leggere 8 MiB**:
- `0x800000` = **8.388.608 byte = 8 MiB**
- `0x8000000` = **134.217.728 byte = 128 MiB**

Soltanto dopo aver validato modello, tool, alimentazione e connessioni sulla GW006,
i due comandi di riferimento in ambiente Windows, con porta COM verificata, sono:

```powershell
py -3 rtltool.py -p COM5 -b1000000 rf 0 0x800000 gw006_lettura_1.bin
py -3 rtltool.py -p COM5 -b1000000 rf 0 0x800000 gw006_lettura_2.bin
```

`COM5` è SOLO un esempio, non la porta già verificata.
Le due letture devono essere operazioni fisiche separate, idealmente con nuovo ingresso in modalità lettura fra le due, senza avviare una funzione di erase/write.
Non lanciare questi comandi finché il pinout del tuo PCB e il programma impiegato non sono stati verificati.

### Confronto automatico sul computer
Dopo due letture **complete**, nella cartella con lo script e i file:

```bash
python3 scripts/verify_gw006_dual_backup_readonly.py gw006_lettura_1.bin gw006_lettura_2.bin --json gw006_verifica_pubblicabile.json
```

Se i file differiscono o non sono lunghi esattamente 8 MiB, l'uscita è `BLOCKED`. Se
coincidono, l'uscita è `TWO_FILES_MATCH_READ_ONLY` con `flash_allowed:false`;
l'algoritmo NON certifica una partizione sicura.

**Conserva i due .bin privatamente in due supporti separati.** I dump possono contenere SSID/password Wi-Fi, credenziali Tuya/OpenBeken, identificatori di dispositivo e altri segreti.
**Non caricarli su GitHub e non inviarli pubblicamente.**
Per un primo controllo di completezza è sufficiente condividere SOLO il JSON privo di dati privati, se desiderato.

## Mappa partizioni: prova separata obbligatoria
Il confronto byte-per-byte dimostra soltanto l'identità delle due copie.
La mappa effettiva richiede dati del bootloader e dell'OpenBeken 1.18.321 presenti
sulla **stessa GW006**, inclusi firmware principale, eventuali slot OTA, NVS/configurazione, Zigbee e calibrazione.

Il codice CP19 `scripts/audit_gw006_flash_map.py` rifiuta dati non classificati,
sovrapposizioni, partizioni troppo piccole e SHA errati, ma NON può provare
l'autenticità della dichiarazione della mappa. Nessun settore completamente `FF`
è automaticamente libero.

La proposta precedente `0x200000..0x6FFFFF` è **non autorizzata**: in 4 dump pubblici
contiene da 606 a 646 settori non cancellati; non usarla come partizione IR.

Il budget caso peggiore del journal è **4.378.624 byte**, senza margine
sufficiente per aggiornamenti illimitati, wear leveling, spazio OTA e backup.
Nessuna regione Flash della scheda personale è certificata oggi.

## Stato di sicurezza CP43
- `GWIR_WBRG1_STORAGE_APPROVED=0` (da mantenere)
- `main` e OpenBeken **non modificati**
- Nessuna comunicazione con hardware nel test CI.
- Nessun ripristino collaudato, quindi nessun file firmware pronto da scrivere.
- Devono ancora essere convalidati: bootloader/OTA, recovery hardware, pin IR e HTTPS.

### Evidenze
- Evidenza pubblica confrontata: `docs/GW006_PUBLIC_WBRG1_FLASH_EVIDENCE_2026-11-14.md`
- Audit base CP19: `scripts/audit_gw006_flash_map.py`
- Audit gemello CP43: `scripts/verify_gw006_dual_backup_readonly.py`
- Test soltanto sintetici: `tests/test_gw006_dual_backup_readonly.py`
- CI: `.github/workflows/gw006-cp43-dual-backup-audit.yml`
