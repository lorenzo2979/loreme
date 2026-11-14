# Checkpoint CP39 — GW006 IR Universal (14 novembre 2026, ore 12:20 CET)

## Obiettivo confermato
Firmware **autonomo per GW006 / Tuya WBRG1 / Realtek RTL8721CSM**, distinto da **OpenBeken 1.18.321**. Nessun flash né alterazione di OpenBeken durante sviluppo. La UI finale deve essere **via browser**, **senza app Android obbligatoria**.

- Quattro telecomandi, 96 tasti totali: TV 36, clima 14, decoder 32, speaker 14.
- Fino a **5 pacchetti IR per tasto**: cattura, prova, trasmissione, memorizzazione e cancellazione; persistenti al riavvio.
- TX **PA25** (OpenBeken 25), RX **PA26** (OpenBeken 26): numerazione scelta, **non prova del cablaggio fisico**.
- Wi-Fi APConfig avviabile dal pulsante: SSID temporaneo aperto GW006-IR-SETUP, portale http://192.168.4.1; scansione SSID, DHCP/IP statico, DuckDNS. Token DuckDNS esclusivamente su canale attendibile, **mai** in chiaro su AP aperto/HTTP.
- Dashboard locale autenticata, anti-replay e provisioning chiave univoca per dispositivo. I segreti di altri dispositivi non devono essere acquisiti/copiati.

## GitHub: stato verificato
- Repository: https://github.com/lorenzo2979/loreme
- `main`: `5892d2dd96a50be19a65d87a385fdb5a9ef1fc97` (**invariato**).
- Branch di lavoro: `review/gw006-realtek-flash-hal`.
- HEAD verificato prima di questo checkpoint: `eae18b1ed124e3ad8a9ce92133eaa5bf649569eb`.
- Confronto col main: **87 commit avanti, 0 indietro** alla verifica; non fondere in main finché non superati i gate.

## Prove positive, non equivalenti alla release
1. Workflow di riferimento GW018-DM `43589015600` del 14/11/2026: **SUCCESS**. KM0 e KM4 nativi compilati, ingresso `gwir_gw018_ir_boot_prepare`, core IR, RX-worker e TX-bus individuati nell'**ELF finale**. Riferimento: https://github.com/lorenzo2979/loreme/actions/runs/43589015600 .
2. La compilazione di riferimento include **12 sorgenti C** (3 core, 8 driver Realtek, 1 boot bridge). Nell'implementazione di riferimento non vengono autorizzati pin IR, archivio Flash, web autenticato e salvataggio dei 96 tasti; le immagini sono marcate **DO NOT FLASH**.
3. In `gwir_gw018_ir_boot.c` è presente un `HELLO` locale al motore IR; il buffer del motore 5×2048 è nella sezione `.psram.bss` dell'SDK di riferimento. La disponibilità effettiva della PSRAM sul GW006 reale non è ancora certificata.
4. Workflow completo SDK ufficiale AmebaD `43583624928`: KM0, KM4, test HAL/boot/identità/APConfig **passati**, ma **job finale fallito al gate del linker**. Riferimento: https://github.com/lorenzo2979/loreme/actions/runs/43583624928 .
5. Motivo del fallimento: **11 moduli compilati ma senza sezioni allocate nell'immagine ELF**: `gwir_amebad_board_pins`, `gwir_apconfig_http`, `gwir_apconfig_http_stream`, `gwir_apconfig_tcp_transport`, `gwir_dashboard_asset`, `gwir_dashboard_http`, `gwir_dashboard_sta_server`, `gwir_network_profile`, `gwir_sdk_sta_dashboard`, `gwir_stream`, `gwir_wbrg1_flash_port`. Non confondere presenza negli oggetti con esecuzione reale.

## Blocchi per un firmware installabile
**B1 — Runtime prodotto**: connettere per davvero boot sicuro, provider chiave/nonce, APConfig, Wi-Fi STA, endpoint HTTP autenticati, dashboard, IR engine e storage. Non basta forzare il linker: devono esserci chiamate effettive e comportamento dimostrabile.
**B2 — Flash reale**: archivio 96×5 nel caso peggiore **4.378.624 byte**; partizione sull'esemplare GW006 non verificata. Audit di quattro dump pubblici non autorizza l'uso di alcuna regione di scrittura. `GWIR_WBRG1_STORAGE_APPROVED=0` resta vincolante.
**B3 — Hardware e recovery**: backup reale non invasivo e ripristino verificato, mappa/bootloader e layout immagini, connessioni RX PA26/TX PA25, eventuale conflitto LED su PA25, PSRAM e tempi del motore IR.
**B4 — Wi-Fi e sicurezza**: APConfig su pulsante, scansione reti, DHCP/statico con rollback, DuckDNS con provisioning sicuro, persistenza segreti e replay-epoch, interfaccia web autenticata.
**B5 — Test funzionali**: acquisizione/transmissione IR reali, 96 codici × 5 frame, riavvio/perdita alimentazione, saturazione, conservazione, rifiuto comandi non autenticati.

## Prossima sequenza tecnica breve
1. Usare come base di verifica l'ultima build **GW018 nativa riuscita**, senza attribuirle compatibilità GW006 e senza copiarne il bootloader o i dati.
2. Collegare prima **HTTP/dashboard/auth in maniera reale e fail-closed** al KM4, mantenendo disabilitati GPIO e Flash non verificati. Verificare con ELF e test di richiesta/autenticazione (non con link forzato).
3. Collegare boot persistente e APConfig **solo tramite board provider validato**, senza credenziali di fabbrica o token su AP HTTP. Rieseguire gate SDK 43 sorgenti e capire gli esclusi rimanenti.
4. Solo con **backup, mappa flash, pinout e ripristino della GW006 specifica** valutare una variante hardware installabile. **Non installare** le immagini diagnostiche attuali.

## Regola di promozione
Niente denominazione «firmware pronto», «installabile», «definitivo» finché mancano runtime completo e verifiche hardware. Nessuna modifica a `main` né a OpenBeken 1.18.321 senza esplicita e successiva approvazione.
