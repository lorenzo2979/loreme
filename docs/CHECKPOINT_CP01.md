# GW006 IR Universal — Checkpoint CP-01
**Data:** 11 novembre 2026  
**Repository:** `lorenzo2979/loreme`  
**Stato:** documentazione e impostazione del progetto. Nessun nuovo firmware Realtek compilato o verificato su hardware.

## 1. Obiettivo
Sviluppare un sistema telecomando IR per la scheda Tuya RSH-GW006 (WBRG1 / Realtek RTL8721CSM), con acquisizione e trasmissione dei codici e quattro telecomandi: TV, Clima, Decoder, Speaker.

L'app Android è una possibile interfaccia principale per la gestione dei telecomandi e delle memorie IR. Esiste anche una precedente proposta con dashboard web sul dispositivo, pagina Config, MQTT e DuckDNS. L'architettura software definitiva per GW006 deve essere confermata **prima** della scrittura del firmware: non trattare come già implementata né l'una né l'altra.

## 2. Hardware e stato osservato
- Scheda: Tuya RSH-GW006.
- Modulo Wi-Fi / SoC: WBRG1, Realtek RTL8721CSM (famiglia AmebaD).
- Il modulo Zigbee ZS3L è stato rimosso.
- Alimentazione via USB-C; adattatore USB-UART a 3,3 V disponibile.
- Componenti identificati in precedenza: AMS1117, D1, LED blu, LED rosso, pulsante.
- OpenBeken / OpenRTL8720D versione **1.18.321** era stato flashato con successo.
- IP LAN osservato in un test precedente: `192.168.1.28` (può essere cambiato).
- PB22 era stato identificato come LED, PB23 come pulsante.
- OpenBeken mostrava driver IR nei menu, ma **non è provato** che RX/TX funzionino con il cablaggio specifico.

### Piazzole ex ZS3L — riferimento allo schema approvato 08/11/2026
Colonna destra guardando lo schema, contando dal basso:
1. prima piazzola: libera;
2. seconda piazzola: RX;
3. terza piazzola: TX.

Nello schema di lavoro, RX = **PA18** e TX = **PA19**. Queste associazioni vanno verificate elettricamente e confrontate con datasheet/pinmux/periferiche del chip; non basta una prova visiva. GND e 3,3 V erano stati individuati; il 5 V non era stato confermato.

**Riferimento grafico:** `GW006_SCHEMA_DEFINITIVO_APPROVATO.png`, citato nella conversazione precedente ma **non presente in questo repository**. Non inventare né sostituire lo schema senza l'originale.

## 3. Requisiti funzionali desiderati
- Quattro telecomandi: TV, Clima, Decoder e Speaker.
- Acquisizione IR da telecomando fisico per ogni pulsante, riproduzione e cancellazione.
- Supportare codici complessi e più pacchetti; il motore **ESP32-C3 V1.7** usava **5 pacchetti**, ma **non è ancora stato portato né validato** sul Realtek.
- Schema di riferimento dei 96 tasti usato nel progetto ESP32-C3: TV 36, Clima 14, Decoder 32, Speaker 14. Il riuso su GW006 è un requisito di progetto, non una funzione già provata.
- Interfaccia e memorizzazione: da decidere tra app Android (con memoria nell'app) e memorizzazione/device web UI; valutare anche la persistenza in flash del dispositivo.
- Opzioni di configurazione già discusse: Wi-Fi, MQTT broker/user/password, IR TX/RX, DuckDNS. Nessuna credenziale incorporata nel sorgente.
- Sicurezza: autenticazione delle richieste di comando, credenziali/token protetti; scegliere il protocollo e verificare i limiti di RAM/flash prima dell'implementazione.

## 4. Software, esperimenti e ostacoli
- OpenBeken funzionante è l'unico firmware segnalato come installato con successo.
- Tentativo di SDK dedicato denominato `GW006_IR_V2` **fallito** per assenza dell'header `rtl8721d_ir.h`.
- Non è stata dimostrata la disponibilità di un SDK Realtek completo, di header IR compatibili, di un pinmux IR valido o di una build riproducibile.
- Non è stato generato, compilato, flashato o testato un firmware GW006 IR Universal personalizzato.
- Un backup del binario OpenBeken era stato dichiarato disponibile dall'utente; **non è stato caricato in GitHub**. Conservare una copia verificata e una procedura di ripristino prima di nuovi flash.

## 5. Repository GitHub
- Account collegato: `lorenzo2979`.
- Repository accessibile: `https://github.com/lorenzo2979/loreme` (pubblico).
- Al collegamento del giorno 11/11/2026 risultava vuoto. La creazione di questo file costituisce avvio della documentazione.
- Non caricare password Wi-Fi, chiavi MQTT, token GitHub, dati privati o dump contenenti segreti.
- Non caricare il binario di backup pubblico senza verificarne provenienza, contenuto e autorizzazione alla distribuzione.

## 6. Passi successivi — ordine vincolante
1. Mantenere intatto OpenBeken sul dispositivo; confermare la presenza del backup ripristinabile.
2. Recuperare l'immagine originale dello schema e verificare elettricamente RX/TX e relativi GPIO/pinmux.
3. Studiare fonti, sorgenti e toolchain Realtek/OpenBeken; verificare quale driver IR supporti davvero RTL8721CSM.
4. Scegliere l'architettura app-only vs interfaccia web/device storage, e precisare il formato di codifica/memorizzazione dei cinque pacchetti.
5. Creare codice e test offline; compilare una build riproducibile per WBRG1.
6. Collaudare prima TX e poi RX con misure e log; verificare acquisizione, riproduzione e persistenza.
7. Valutare il flash del nuovo firmware **solo** dopo backup e procedura di recupero accertata.

## 7. Separazione dal progetto ESP32-C3
Il progetto IR Universal C3 è distinto: ESP32-C3 Super Mini 4 MB, ESP-IDF 5.5.6, RX GPIO21, TX GPIO5, motore IR V1.7 e 96 pulsanti/5 pacchetti. I suoi risultati e la sua architettura non possono essere dati per automaticamente compatibili con Realtek RTL8721CSM.

## 8. Regole di avanzamento
- Distinguere sempre **desiderato**, **proposto**, **compilato**, **testato**, **funzionante sul dispositivo**.
- Registrare ogni prova con versione, file sorgenti, parametri, log ed esito.
- Non cambiare il cablaggio approvato o il firmware funzionante senza esplicita decisione e una prova sufficiente.
