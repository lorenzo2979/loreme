# GW006 IR Universal — Checkpoint CP-03 / REV7
**Data:** 11 novembre 2026
**Progetto:** GW006 IR Universal V2, Tuya GW006 / WBRG1 (RTL8721CSM).

## Risultato concreto REV7
Implementato in C un **gestore di invio infrarossi a blocchi** per la FIFO TX di 32 parole del controller AmebaD. Gestisce fino a 2048 parole codificate per frame, controlla lo spazio libero, segnala il bit END sull'ultima parola, avvia una sola volta la periferica e considera completato l'invio solo quando l'ultima parola è stata accodata, la FIFO è vuota e la macchina a stati non risulta più attiva. Include un adattatore alle funzioni SDK `IR_GetTxFIFOFreeLen`, `IR_SendBuf`, `IR_Cmd`, `IR_FSMRunning`.

**Non** sono stati implementati configurazione dei pin o clock/carrier, ricezione IRQ, TCP Wi-Fi, autenticazione, build/link dell'SDK o flash della GW006. Nessuna prova hardware.

## Test realmente eseguiti
- `make test` con GCC: PASS (core IR, protocollo, stream TCP, codec TX, nuova FIFO TX, loopback TCP).
- `make CC=clang ... -fsanitize=address,undefined test`: PASS.
- `make test-kotlin`: PASS per protocollo GWIR3, ACK, 96 pulsanti, memoria di 5 frame.
- `make arm-smoke`: PASS e sei file oggetto EABI ARM (3 moduli × 2 CPU). I target giusti sono **Cortex-M23 (KM0)** e **Cortex-M33 (KM4)**, architettura Armv8-M, secondo datasheet RTL872xCS. I file oggetto sono **non collegati** e l'adattatore usa uno **stub di test `ameba_soc.h`**, non l'SDK originale. Non sono firmware.

## Revisione importante dei pin
Il datasheet ufficiale Tuya WBRG1:
https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz

conferma PA18 come **UART0_TX/TX0** e PA19 come **UART0_RX/RX0**. La precedente ipotesi PA19 TX IR / PA18 RX IR non è una coppia IR nativa documentata. Gli esempi AmebaD propongono **PA25 TX / PA26 RX** oppure **PB23 TX / PB22 RX**, che però possono avere conflitti con l'hardware GW006 (LED/pulsante). Esiste anche una discrepanza tra commenti di un esempio e altra documentazione nel verso della coppia PB29/PB31. **Non riassegnare o ricablare pin senza verifica elettrica del PCB.**

## Pacchetto consegnato all'utente
`GW006_IR_NATIVE_V2_REV7_TX_FIFO_ARM_OBJECTS.zip` — 55 file, 67.363 byte, archiviazione ZIP integrità OK.
SHA256: `8c2b2e0b853a5160273dbe3cdc091e7be8758450264418ea7947901df453c4a8`.
Contiene codice REV5/REV6/REV7, test, documentazione, script ricompilazione ARM, manifest SHA256 e sei oggetti ARM **NON FLASHABILI**.
**Lo ZIP è stato creato nella conversazione; il repository GitHub contiene per ora i checkpoint, non l'archivio integrale dei sorgenti.**

## Stato e prossima fase
- OpenBeken 1.18.321 funzionante sul dispositivo: **inalterato**.
- Nessun `.bin` installabile, nessun APK, nessuna prova su scheda.
- Prossimo: SDK AmebaD completo + build reale KM0/KM4, conferma dei pin accessibili e della periferica, HAL RX, pianificazione task/refill TX con scadenze, Wi-Fi TCP con autenticazione e anti-replay, backup e recovery verificati prima di ogni flash.
- La memoria dei 96 pulsanti rimane un requisito dell'app Android; non confondere il progetto GW006 con ESP32-C3.

## Fonti tecniche
- SDK corretto: https://github.com/Ameba-AIoT/ameba-rtos-d
- Header IR: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h
- Esempio RX: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_rx_learning/src/main.c
- Esempio TX: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_tx_led/src/main.c
- Datasheet architettura: https://www.realmcu.com/img/ipa/en_637914076404583067.pdf

**Non eseguire flash senza build integrata, verifiche di circuito e rollback garantito.**
