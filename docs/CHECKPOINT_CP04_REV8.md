# GW006 IR Universal — Checkpoint CP-04 / REV8
**Data:** 11 novembre 2026  
**Repository:** `lorenzo2979/loreme`

## Avanzamento concretamente verificato
- Ripartito dai sorgenti **REV7** recuperati in questa conversazione; creato il pacchetto **REV8**.
- Aggiunta una pipeline RX in C, `gwir_amebad_rx_fifo.{c,h}`: FIFO software da 512 eventi con produttore ISR e consumatore task, livello IR HIGH/LOW, conversione tick→µs, rilevamento fine frame, rifiuto dei frame parziali dopo overflow/errore.
- Aggiunto l'adattatore delle funzioni reali Realtek `IR_GetRxDataLen()` e `IR_ReceiveData()` (`gwir_amebad_rx_vendor.{c,h}`). I **test** usano una stub `ameba_soc.h`, NON l'SDK reale.
- Integrato il nuovo modulo RX con `gwir_capture_builder` e `gwir_engine`, che ricevono più frame con ACK prima di rimuoverli dalla coda.
- La frequenza portante in ricezione è una **stima configurabile**, non una lettura certa da un sensore IR demodulato.

## Compilazioni e prove eseguite
- `make test` (GCC): **PASS**, inclusi core, protocollo, TX codec, TX FIFO, RX FIFO e TCP localhost.
- `make CC=clang ... -fsanitize=address,undefined test`: **PASS**, nessun problema segnalato dai sanitizzatori.
- `make test-kotlin`: **PASS** sulla JVM, non è un APK.
- `make arm-smoke`: **PASS**, 10 oggetti ARM ELF32 compilati per **Cortex-M23 (KM0)** e **Cortex-M33 (KM4)** usando stub SDK.
- Coperti casi RX: polarità attiva bassa/alta, fine frame per idle o SPACE lunga, errori, saturazione della coda, sincronizzazione dopo perdita di campioni, FIFO hardware simulata da 32 campioni, oltre 900 eventi che verificano il riavvolgimento della coda.

**Dimensioni misurate sul computer di prova, non ancora sul chip:** `sizeof(gwir_engine)=49280`, `sizeof(gwir_amebad_rx_queue)=4108`, `sizeof(gwir_amebad_rx_worker)=8280` byte, circa 61,7 KB complessivi esclusi TX, pile task e allocazioni di rete.

## Archivio consegnato in questa chat
`GW006_IR_NATIVE_V2_REV8_RX_FIFO_ARM_SRC.zip`
- File: **68**.
- Dimensione: **86610 byte**.
- SHA-256: `05de7a53f701f60a750e1822c862420596fecec0144599a926c842d8c45c8d3e`.
- Archivio ZIP e manifesto SHA256 verificati; conserva sorgenti, report e oggetti ARM **non flashabili**.
- **GitHub conserva i checkpoint, non ancora tutto lo ZIP sorgente**.

## Ostacoli ancora presenti
1. Nessuna compilazione integrata/link nell'SDK `Ameba-AIoT/ameba-rtos-d` per la GW006; l'ambiente locale non può clonare il repository remoto.
2. Interfaccia IRQ FreeRTOS reale, clock del ricevitore, polarità e periferica IR nativa **non testati sul dispositivo**.
3. Il vecchio schema **PA19 TX / PA18 RX** indica pad UART del WBRG1 e non dimostra supporto IR; pinmux e collegamenti reali ancora da verificare. Non ricablare.
4. Sicurezza Wi-Fi/TCP e anti-replay non implementati; CRC non è autenticazione. Nessun APK realizzato.
5. Flash e procedura di ripristino da verificare prima di qualsiasi installazione.

## Regola essenziale
**Non flashare i .o o i sorgenti REV8.** OpenBeken 1.18.321 installato sulla scheda rimane intatto.

## Riferimenti
- Realtek SDK: https://github.com/Ameba-AIoT/ameba-rtos-d
- IR header: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h
- RX example: https://github.com/Ameba-AIoT/ameba-rtos-d/blob/main/project/realtek_amebaD_va0_example/example_sources/ir/raw/ir_rx_learning/src/main.c
- WBRG1 datasheet: https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz
