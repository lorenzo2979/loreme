# GW006 IR Universal — CHECKPOINT CP-05 / REV9

**Data:** 11 novembre 2026. **Repository:** lorenzo2979/loreme.

## REV9: integrazione nel progetto ufficiale AmebaD

Ho creato uno script di integrazione reversibile (scripts/integrate_amebad_sdk.py) per il repository ufficiale Ameba-AIoT/ameba-rtos-d. Copia 10 moduli C di GW006 nel progetto KM4 e modifica soltanto i file Realtek src/src_hp/Makefile e src/src_hp/main.c con blocchi marcati. Aggiunge un messaggio diagnostico all'avvio; NON abilita i pin, le periferiche IR, la connessione di rete o un server TCP.

Lo script permette dry-run, apply, remove, backup del codice Realtek, rifiuto conflitti e controllo di SDK mancanti. I percorsi sono verificati nelle fonti ufficiali:
https://github.com/Ameba-AIoT/ameba-rtos-d

## Test eseguiti

- PASS: GCC per protocollo GWIR v3, core IR, TX FIFO, RX FIFO, loopback TCP simulato.
- PASS: Clang con AddressSanitizer e UndefinedBehaviorSanitizer.
- PASS: Kotlin/JVM per i 96 pulsanti, 5 frame e ACK; NON è un APK.
- PASS: compilazione ARM Cortex-M23/M33 con stub di test; oggetti NON collegati.
- PASS: test integrazione SDK simulato, 11 oggetti C, dry-run, applicazione ripetuta, rollback e rifiuto conflitti.
- NON ESEGUITA: compilazione e collegamento del vero SDK KM0/KM4. Il clone completo Realtek non è accessibile da questo container (DNS github.com bloccato).
- NON ESEGUITI: flash, test GPIO e acquisizione/trasmissione su GW006 fisica.

## Archivio REV9 prodotto

**GW006_IR_NATIVE_V2_REV9_SDK_INTEGRATION_SRC.zip**
75 file; 99.177 byte.
SHA-256: 7b788ca95988e88bfb8c79dfcda7d55bfdd7100e9cf556c387c1e5e46545b6d2
Manifest SHA256 di tutti i 74 altri file dell'archivio verificato.

Il file ZIP è disponibile nella conversazione. **Il repository GitHub conserva i checkpoint, ma NON ancora tutto il codice sorgente REV9.**

Incluso workflow GitHub Actions manuale sdk-native-rev9.yml: per funzionare richiede che tutti i sorgenti REV9 siano presenti nel repository. Può tentare il clone SDK e il build reale KM0/KM4 sul runner, ma NON è stato eseguito; carica solo i log.

## Limiti di sicurezza e hardware

Hardware: Tuya RSH-GW006 / WBRG1 / RTL8721CSM.
OpenBeken 1.18.321 installato: **invariato**.
Pin PA18/PA19 identificati come UART nel datasheet del WBRG1: supporto IR hardware non verificato. Non ricablare.
TCP Wi-Fi reale, autenticazione, anti-replay, driver IR completi e APK Android: non ancora implementati.
CRC non costituisce autenticazione. Non esiste un BIN GW006 personalizzato pronto al flash.

## Prossimo passo

Pubblicare tutti i sorgenti REV9 nel repository GitHub, attivare manualmente il workflow SDK, analizzare i log del toolchain Realtek e correggere ogni errore, senza effettuare flash fino alla verifica di pinmux, backup e recovery.