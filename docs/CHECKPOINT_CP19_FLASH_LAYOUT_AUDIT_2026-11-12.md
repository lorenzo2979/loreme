# GW006 IR Universal — CP-19 | Flash Layout Gate (12/11/2026)

**NON FLASHARE.** Solo verifica su file locali, nessuna lettura o scrittura di hardware.
OpenBeken 1.18.321 rimane inalterato. Questa verifica NON è un permesso di programmazione.

## Perché serve
GWCD v1 per 96 pulsanti e 5 frame di 2048 impulsi richiede 4.333.568 byte nel caso estremo, compresi i due settori indice, senza spazio per un aggiornamento. Con un singolo aggiornamento massimo (45.056 byte) il minimo sale a **4.378.624 byte**. Restano da considerare firmware, OTA, configurazione, calibrazioni e ulteriori aggiornamenti. L'archivio CP-18 non dispone di compattazione o wear leveling: una partizione che supera il solo minimo può esaurirsi rapidamente.

## Cosa abbiamo realizzato
`scripts/audit_gw006_flash_map.py` controlla **esclusivamente in lettura** un documento JSON sulla mappa della scheda e un backup integrale: identificazione GW006/WBRG1, dimensioni e allineamenti dei settori, SHA-256 del file di backup, aree dichiarate riservate, assenza di intersezioni e assenza di zone non classificate. Verifica la capienza minima e restituisce solo `LAYOUT_PLAUSIBLE_NO_WRITE_AUTHORIZATION`.

Il controllo non può stabilire che la mappa dichiarata sia autentica o completa: occorre ricostruirla e verificarla fisicamente con dati ufficiali del firmware realmente installato. Un file sintetico può superare i test, ma non autorizza scritture. Il programma NON comunica con GW006, UART, JTAG o flash.

## Uso — quando esisterà un backup verificato
1. Eseguire il backup completo della flash con un metodo di sola lettura validato e salvarlo in due copie.
2. Documentare l'esatta mappa reale delle partizioni nel modello `docs/flash_layout_TEMPLATE_BLOCKED.json`, modificandolo solo con valori verificati, non ipotizzati.
3. Calcolare il vero `sha256` del backup e compilare `backup_sha256`, annotando fonte e metodo della verifica.
4. Eseguire da PC: `python3 scripts/audit_gw006_flash_map.py layout_reale.json --backup backup_reale.bin`.
5. Anche se il rapporto è positivo, NON autorizza il flash. Verificare l'interazione con OTA, bootloader e i driver, poi completare un recovery collaudato.

## Test CP-19
`python3 tests/test_flash_map_audit.py`: **7 casi PASS** (synthetic map, mappa incompleta, overlap, overflow, identità errata, prove mancanti, controllo CLI e backup). 
`make test-flash-journal test-gwcd-store test-sdk-integration`: PASS su host; SDK Realtek reale non compilato. Nessuna scrittura su GW006.

### Prossimo gate
Build/link KM0/KM4 reale; sorgenti CP-18/CP-19 da sincronizzare integralmente su GitHub; backup e verifica della partizione della scheda; backend fisico con limiti, protezione e GC; API web IR autenticata. La dashboard web sul chip resta sola lettura finché queste condizioni non saranno soddisfatte.