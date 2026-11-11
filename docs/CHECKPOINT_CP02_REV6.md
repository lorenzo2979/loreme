# GW006 IR Universal — Checkpoint CP-02 / REV6
Data: 11 novembre 2026.

## Nuovi risultati verificati
- Archivio sorgenti REV5 originale recuperato dalla Libreria; i test **host GCC e Clang con AddressSanitizer/UBSan** hanno avuto esito positivo.
- Test **Kotlin/JVM** delle memorie dei 96 pulsanti, 5 pacchetti e staging/ACK superati.
- Creato un **modulo C autonomo di conversione microsecondi -> parole della FIFO TX Realtek**, con test host superati (REV6).
- L'SDK ufficiale **Ameba-AIoT/ameba-rtos-d** contiene il file **`component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h`** e le funzioni `IR_SendBuf`, `IR_ReceiveBuf`, `IR_Init`. Il vecchio problema dell'header mancante dipendeva dalla copia/toolchain locale, non dall'assenza nel repository ufficiale.
- Gli esempi ufficiali del controller IR indicano le coppie TX/RX **PA25/PA26, PB23/PB22, PB29/PB31**. Il cablaggio di lavoro **PA19 TX/PA18 RX non è verificato per la periferica IR nativa**.

## Stato reale della compilazione
**Test eseguibili su PC: compilati e superati. Firmware installabile RTL8721CSM: NON compilato.**

Restano da realizzare: HAL RX (ISR/FIFO), streaming FIFO TX, task FreeRTOS, trasporto TCP/Wi-Fi, autenticazione/anti-replay, integrazione KM0/KM4, verifiche pinmux sul PCB e backup completo del dispositivo. Non esiste un APK finale o un binario personalizzato verificato.

## Archivio sorgenti prodotto in questa sessione
`GW006_IR_NATIVE_V2_REV6_REALTEK_TX_CODEC_SRC.zip` (52 KB circa; conservato come allegato nella chat, **non caricato su questo repository**).
SHA256: `349a8f0d2ef40fad1094dafd8617f46ec606b90791e9f9c783999105218eee79`

Il pacchetto contiene i sorgenti REV5, il nuovo codec, i test, un manifest SHA256 per i file e la documentazione sulla ricerca nell'SDK.

## Fonti SDK e riferimento testuale
- https://github.com/Ameba-AIoT/ameba-rtos-d
- https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h
- https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_rx_learning/src/main.c
- https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_tx_led/src/main.c

## Vincolo di sicurezza
**Lasciare OpenBeken 1.18.321 installato sul GW006 completamente intatto.** Non scrivere nuovi binari, non assegnare nuovi pin, non cambiare configurazione finché non saranno verificati pinmux, immagine e percorso di ripristino. Il protocollo GWIR v3 attuale usa CRC per errori accidentali: non è ancora autenticato e non va esposto in LAN reale.
