# CHECKPOINT CP43 GW006 — backup integrale, audit Flash e blocco di sicurezza
Data: 14 novembre 2026

## Stato effettivo
**L'AUDIT SOFTWARE È STATO REALIZZATO E TESTATO. L'AUDIT FISICO È BLOCCATO PER MANCANZA DEL BACKUP PERSONALE.**

- Scheda prevista: RSH-GW006, Tuya WBRG1, RTL8721CSM, Flash nominale 8.388.608 byte.
- L'utente possiede OpenBeken 1.18.321 da preservare; nessuna azione in questa esecuzione ha comunicato con la scheda, con la porta UART, scritto o cancellato flash.
- Repository `lorenzo2979/loreme`, branch `review/gw006-realtek-flash-hal`; `main` invariato `5892d2dd96a50be19a65d87a385fdb5a9ef1fc97`.
- Ricerca eseguita nei file disponibili e in GitHub: esistono sorgenti, report e immagini KM0/KM4 NON INSTALLABILI, e analisi di quattro dump PUBBLICI di altri prodotti WBRG1; **non è disponibile alcuna coppia di backup integrali ottenuti dalla GW006 personale**. Questo impedisce di attestare l'effettivo layout della scheda.

## Completato ora (codice e test)
- `scripts/verify_gw006_dual_backup_readonly.py` — compara due file di **8 MiB esatti** in lettura e per ogni byte; calcola SHA256, respinge file identici per percorso/inode, file troncati, byte divergenti e immagini uniformi; riporta statistiche per settore, ma considera **non certificata** qualunque zona cancellata 0xFF.
- Rileva eventuali settori NON 0xFF nell'area `0x200000..0x6FFFFF` precedentemente proposta, senza mai approvarla o scriverci.
- Uscita pubblicabile `--json` priva di contenuto Flash, credenziali e nomi dei file; nessun accesso seriale o rete, nessuna funzione di flashing.
- In **tutti** i risultati, incluso il confronto perfettamente identico: `flash_allowed:false`, `real_partition_layout_verified:false`, `physical_independent_reads_proven:false`, `bootloader_and_recovery_verified:false`.
- `tests/test_gw006_dual_backup_readonly.py`: dati sintetici da 8 MiB, stessa origine, hardlink, mismatch, troncamento, all-FF, hash, nessuna scrittura sugli originali, redazione della risposta.
- `.github/workflows/gw006-cp43-dual-backup-audit.yml` — GitHub Actions SUCCESS https://github.com/lorenzo2979/loreme/actions/runs/43600194519; test sintetici **non attestano una lettura hardware**.
- `docs/GW006_CP43_BACKUP_REALE_SOLO_LETTURA.md` — istruzioni per due acquisizioni indipendenti e confronto su PC, con la correzione dell'errore comune nella documentazione: 8 MiB è `0x800000`, NON `0x8000000` (128 MiB). Niente funzione "backup and flash" automatica.
- `docs/flash_layout_GW006_REAL_BOARD_BLOCKED_TEMPLATE.json` — modello che FALLISCE finché manca la mappa reale, senza indirizzi di partizione inventati.

## Stato del firmware e condizioni ancora mancanti
1. Memoria persistente 96×5 caso peggiore: **4.378.624 byte** come minimo del journal, ma la zona reale libera **non è stata verificata**; settori FF non sono prova di libertà (OTA/riservati).
2. Quattro dump pubblici WBRG1 hanno da 606 a 646 settori NON cancellati nell'area vecchia di 5 MiB: nessuna autorizzazione a usarla.
3. Backup OpenBeken fisico: **MANCANTE**; due letture UART indipendenti con log e salvataggio privato necessari.
4. Mappa bootloader/OTA/configurazione/certificati/codici Zigbee e recupero hardware: **NON VERIFICATI**.
5. HTTPS/provisioning identità unica, APConfig e test IR PA25/PA26 sulla vera scheda: **NON COMPLETI**. Final linker Realtek CP42 aveva 9 moduli runtime non allocati.
6. `GWIR_WBRG1_STORAGE_APPROVED=0` deve restare tale. Vietato aggiornare `main` o flashare un'immagine sperimentale.

## Come sbloccare il controllo reale
La GW006 deve essere raggiunta fisicamente da computer e adattatore UART a logica 3,3 V, oppure deve essere consegnato a un tecnico l'incarico **esclusivo di lettura**.
Identificare fisicamente il PCB e il connettore; usare una funzione documentata read flash (non flash/download/write), acquisire due backup da 8.388.608 byte in due sessioni reali separate; non condividere pubblicamente i dump che contengono password e token.
Eseguire **localmente**:
```
python3 scripts/verify_gw006_dual_backup_readonly.py copia1.bin copia2.bin --json verifica.json
```
Solo dopo valutare manualmente mappa boot/OTA e `scripts/audit_gw006_flash_map.py`, senza attribuire proprietà ai settori FF. L'esito di un audit locale non autorizza comunque a programmare la scheda senza test recovery completo.

## Sintesi
**CP43 = CONTROLLO AUTOMATICO DEL DOPPIO BACKUP PRONTO E TESTATO; BACKUP REALE ASSENTE; FIRMWARE NON INSTALLABILE.** 
