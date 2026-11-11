# CHECKPOINT CP-07 — GW006 IR Universal

**Data:** 11 novembre 2026  
**Scopo:** stato consolidato e punto di ripartenza, con distinzione tra sorgenti, test e funzioni realmente operative sulla scheda.

## 1. Identità e sicurezza della scheda

- Dispositivo: Tuya RSH-GW006, modulo WBRG1 / Realtek RTL8721CSM (AmebaD).
- Sul dispositivo è installato e funzionante **OpenBeken 1.18.321**. Non è stato flashato alcun firmware GW006 IR Universal personalizzato.
- Repository: https://github.com/lorenzo2979/loreme, branch `main`.
- Ultimo commit pubblicato verificato prima del checkpoint: `fc231e3161c66cb04e366b5bb7a7f05f9a5d2207` (**REV14**).
- Compilazione nativa REV14 KM0 + KM4 riuscita: https://github.com/lorenzo2979/loreme/actions/runs/43058177860.
- REV13 Mbed TLS/HMAC: compilazione SDK riuscita: https://github.com/lorenzo2979/loreme/actions/runs/43056285697.
- Le build SDK **non** dimostrano compatibilità del layout flash GW006, correttezza dei GPIO, test su hardware reale o sicurezza d'installazione.

## 2. Revisione in corso: REV15

- La **REV15 APConfig OPEN** esiste come **sorgenti locali e archivio ZIP**: `GW006_IR_NATIVE_V2_REV15_APCONFIG_OPEN_SRC.zip`.
- Sorgenti e test locali aggiornati: Wi-Fi AP senza password mediante `RTW_SECURITY_OPEN`, puntatore password `NULL` e lunghezza `0`; gestione sessione/errore e timeout; test host, Kotlin, ARM compile-only e integrazione SDK simulata (21 oggetti) superati.
- **REV15 non ancora pubblicata su GitHub e non ancora compilata in CI con l'SDK Realtek ufficiale** al momento del checkpoint.
- Il portale di provisioning reale, server HTTP, DHCP, memorizzazione persistente delle credenziali e ritorno affidabile alla rete precedente **NON sono implementati**.

## 3. Requisiti definitivi concordati

### Pulsante PCB e APConfig

- Usare il pulsante fisico del PCB, attribuito provvisoriamente a **PB23** sulla base delle osservazioni OpenBeken; assegnazione elettrica da verificare.
- **Pressione lunga di 5 secondi**: richiedere l'avvio di APConfig.
- Hotspot **aperto**, **senza password Wi-Fi, login o token per la pagina di configurazione**. È attivo solo su richiesta e solo temporaneamente.
- SSID di progetto `GW006-IR-SETUP`; indirizzo previsto `192.168.4.1` (non ancora verificato sulla scheda).
- Fermare APConfig al salvataggio riuscito o **dopo 5 minuti**.
- Non eliminare le vecchie credenziali per la sola pressione del pulsante; se il nuovo collegamento fallisce, prevedere il ripristino della configurazione precedente.
- L'hotspot aperto espone le impostazioni a chiunque sia nelle vicinanze per quei minuti: mantenere il periodo minimo e non esporre funzioni IR o amministrative non necessarie.

### App Android e comunicazione

- Quattro telecomandi: TV **36**, Clima **14**, Decoder **32**, Speaker **14** = **96 tasti**.
- Ogni tasto può gestire fino a **5 pacchetti IR** nel modello applicativo; i codici sono conservati dall'app Android nel progetto GW006.
- **Porta TCP predefinita/fissa: 42424**, sia nell'app sia nel futuro firmware. Attualmente è una **specifica**, non una porta di rete già operativa.
- Collegamento locale tramite IP della GW006, collegamento remoto tramite dominio DuckDNS.
- **DuckDNS richiesto nella configurazione dell'app Android**: dominio, token inserito dall'utente, attiva/disattiva, stato aggiornamento; non è ancora implementato.
- L'accesso remoto richiederà configurazione di rete appropriata (inoltro porta o alternativa compatibile; valutare CGNAT), con autenticazione dei comandi normali. **Wi-Fi APConfig aperto NON significa comandi IR remoti senza autenticazione**.
- Il repository contiene componenti Kotlin/JVM per il protocollo e la gestione codici; **non esiste ancora un APK Android GW006 completo**.

### Motore IR e sicurezza

- Componenti TX/RX, FIFO, interrupt, gestione half-duplex, acquisizione e ACK sono sviluppati e hanno superato test software; alcune parti compilano nell'SDK Realtek.
- GWAS v1 HMAC-SHA256 e protezione anti-replay sono sviluppati; adapter Mbed TLS presente e compilato in REV13; non è ancora un servizio LAN completo con provisioning credenziali funzionante.
- Nessuna chiave Wi-Fi, DuckDNS o HMAC preimpostata nel firmware.

## 4. Vincoli hardware ancora bloccanti

- Nel modulo WBRG1 **PA18/PA19** sono UART0, non una coppia IR nativa confermata. Le precedenti piazzole ex-ZS3L identificate come RX/TX non autorizzano l'attivazione del driver IR su quei pin.
- Le coppie IR viste negli esempi SDK includono PA25/PA26, PB23/PB22, PB29/PB31: nessuna coppia è ancora confermata come accessibile e libera sul PCB GW006.
- Dalle osservazioni OpenBeken, **PB22 LED** e **PB23 pulsante**; non usarli come coppia IR senza verifica fisica e revisione di progetto.
- Prima del flash: confermare pinmux e collegamenti reali, polarità ricevitore, circuito TX e tensioni; fare backup del firmware originale, verificare mappa delle partizioni/bootloader e percorso di recupero.
- Il gate hardware IR deve rimanere **disabilitato** finché queste condizioni non sono risolte.

## 5. Lavoro necessario per arrivare al firmware installabile

1. Pubblicare e compilare la REV15 con l'SDK ufficiale; leggere i log reali.
2. Completare gestione STA/AP, DHCP, HTTP di provisioning, credenziali persistenti, scadenza AP e rollback di rete, senza esporre il motore IR durante APConfig.
3. Completare il servizio TCP sulla porta **42424** con autenticazione dei comandi e gestione sicura delle chiavi.
4. Completare l'app Android GW006 con i 96 pulsanti, la configurazione APConfig e la sezione DuckDNS.
5. Confermare il cablaggio reale TX/RX e il pin del pulsante, eseguire prove elettriche e verificare stabilità RX/TX.
6. Verificare layout flash, backup e recovery prima di generare e qualificare un `.bin` per la GW006 specifica.

## 6. Regola inderogabile per le revisioni successive

**Non flashare la GW006 e non dichiarare il firmware pronto** in base ai soli test host o al successo di GitHub Actions. Preservare OpenBeken 1.18.321 finché hardware, boot, rete, APConfig, recupero e immagini installabili non saranno verificati.

**Ripartenza consigliata:** REV15 OPEN APConfig locale -> pubblicazione GitHub -> build SDK KM0/KM4 -> completamento servizio Wi-Fi e app -> validazione fisica prima del flash.