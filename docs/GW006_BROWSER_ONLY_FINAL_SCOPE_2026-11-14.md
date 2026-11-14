# GW006 IR Universal — decisione architetturale: solo browser, nessuna app Android
Data: 14 novembre 2026
Decisione confermata dall'utente: la GW006 deve funzionare autonomamente con interfaccia web da smartphone/computer, senza APK Android obbligatorio.

## Requisiti invariati (obbligatori)
- Hardware GW006 / Tuya WBRG1 / Realtek RTL8721CSM; SDK ufficiale AmebaD, non ESP32.
- IR TX PA25 (OpenBeken 25), IR RX PA26 (OpenBeken 26).
- 96 tasti: TV 36, climatizzatore 14, decoder 32, speaker 14.
- Fino a 5 pacchetti IR per tasto, con apprendimento, prova, riproduzione, salvataggio ed eliminazione.
- Memoria permanente dopo perdita di alimentazione; capacità calcolata con dati caso peggiore e margine.
- Wi-Fi configurabile (prima connessione AP, poi STA) e accesso locale autenticato.
- **APConfig resta attivabile dal pulsante fisico della GW006**, usando il comportamento previsto dal progetto: il pulsante di configurazione non viene eliminato o sostituito dall'interfaccia web. L'accesso e il salvataggio delle credenziali devono rimanere protetti.
- Quattro telecomandi grafici responsivi, status codici salvati, funzioni di configurazione e diagnostica.
- Rifiutare credenziali di fabbrica condivise, autenticazione assente, contatori anti-replay resettati senza protezione.
- Non usare una partizione Flash da un dump WBRG1 di altri prodotti come se fosse certamente libera sulla GW006.

## Variazione di ambito (accelerazione)
- NON sviluppare, compilare o richiedere una app Android dedicata.
- La UI viene servita direttamente dal firmware GW006 via HTTP locale; tutte le azioni sensibili richiedono autenticazione verificata.
- Possibile collegamento alla schermata Home del telefono tramite browser; nessun APK necessario.
- I componenti HTTP/dashboard/AP già presenti vanno *collegati realmente all'avvio*; non conta includerli forzatamente nel linker.
- Nessuna funzione IR, memorizzazione o configurazione viene rimossa per ottenere questa semplificazione.
- Codice Android storico può restare archiviato per riferimento: non è una dipendenza della release.

## Gate di accettazione della release
1. SDK KM0+KM4 nativo compila con i moduli web/auth/IR/memoria effettivamente richiamati.
2. Mappa del linker conferma presenza delle entry point fondamentali e le sezioni allocate.
3. Accesso locale via browser, provisioning AP/STA, comandi autenticati con risposte, nonce e anti-replay.
4. Acquisizione e trasmissione da PA26/PA25 con 96 tasti e fino a 5 pacchetti.
5. Lettura, scrittura, power-cycle, cancellazione e test di saturazione/conservazione sullo spazio Flash autorizzato della GW006.
6. Backup/ripristino verificati e immagini KM0+KM4 compatibili con bootloader e layout reale GW006.
7. Verifica su hardware reale prima della dicitura INSTALLABILE/DEFINITIVO.

## Blocchi aperti, non mascherare
- Binding avvio reale con credenziali e Wi-Fi/AP/dashboard.
- Chiave per dispositivo e nonce persistente: driver già sviluppato, fornitore sicuro board-specific non integrato.
- Partizione Flash realmente assegnabile e sua capienza: non verificata; scritture intenzionalmente disabilitate.
- Pinmux IR predisposto ma da autorizzare/testare sulla scheda.
- Nessun hardware boot test del firmware autonomo: immagine diagnostica NON INSTALLARE.

La release definitiva non può essere certificata da una build verde o da un .bin prodotto in CI.
