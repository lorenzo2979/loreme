# CP-12 — GW006 IR Universal | 11/11/2026

**Non flashabile.** Questo checkpoint documenta i sorgenti locali verificati, non dichiara un firmware Realtek avviabile su GW006.

## Risultati nuovi (controllati localmente)
- **GPIO candidato principale:** TX **PA25**, RX **PA26**. PA25 è ancora fisicamente collegato al LED rosso sul GW006: richiede isolamento elettrico, misure e backup *prima* di un uso reale. Pin policy convalida la configurazione *solo come metadato* e mantiene `gwir_board_ir_enable_authorized() == 0`. **PB31/PA26** conservato come candidato alternativo non validato.
- **Nuovo parser TCP autenticato:** `firmware/core/gwir_auth_stream.c/.h`. Incapsulamento GWAS v1, record incrementali fino al massimo previsto, HMAC-SHA256 e anti-replay applicati da `gwir_auth_dispatch` prima di passare al motore IR. Rifiuta direzione/magic/nonce/lunghezza errate, HMAC manomessi e replay; chiude in caso di errore. Senza chiavi/nonce sicuri non avvia il servizio.
- **Nuovi test host:** `tests/test_auth_stream.c` e `tests/test_auth_stream_tcp_loopback_posix.c`. Esito PASS su frammenti, invii parziali, 2048 durate, risposte firmate e ripetizioni bloccate; vera socket 127.0.0.1 **sul computer**, non sulla scheda.
- SDK mock: 28 moduli compilati, rollback integrazione testato. ARM Cortex-M23/M33 **compile-only**. Suite completa eseguita dopo `make clean`: **PASS**, inclusi 30.000 input HTTP alterati, ASan/UBSan, dashboard e Kotlin/JVM (96×5 Android).
- I codici IR persistenti appartengono attualmente al componente `AndroidCodeStore`: la dashboard autonoma non ha ancora storage firmware. Non confondere questa implementazione con i 96 tasti già operativi su GW006.

## Pacchetto sorgente, nella conversazione ChatGPT
- `GW006_IR_UNIVERSAL_CP12_SOURCE_ONLY_TESTED.zip`: 153 file di sorgenti, test e documenti, senza immagini flash.
- SHA-256 `8e6c945dd1efd1a1eaf056d2dd5d5cb5c81db12ada563de2ac6083e1b9f96049`.
- Log di prova `GW006_CP12_FULL_CLEAN_BUILD.log`; SHA-256 `d99d36dd0328fb2521d50ff08ab8bf6ed3476d065c661bb6818c108827931047`.
- **Il pacchetto CP-12 completo non è ancora sincronizzato su questo repository**. L'ultima revisione completa del codice su GitHub rimane la REV21 trasferita come delta, con build nativa ancora in coda.

## Bloccanti per ottenere firmware installabile
1. Build nativa KM0/KM4 dei nuovi sorgenti, verifica immagine boot e flash WBRG1. Una build **REV16** era riuscita ([run 43073357001](https://github.com/lorenzo2979/loreme/actions/runs/43073357001)); la build **REV21** è ancora `queued` ([run 43095247514](https://github.com/lorenzo2979/loreme/actions/runs/43095247514)) e non dimostra nulla sulla CP-12.
2. Realtek runtime reale: FreeRTOS task, Wi-Fi STA e APConfig DHCP/HTTP, socket TCP 42424, caricamento sicuro di chiavi HMAC e nonce RNG, connessione ai driver IRQ/pinmux IR e prova 38 kHz.
3. App Android completa e/o dashboard web operativa; se dashboard autonoma senza smartphone, archiviazione flash su gateway con partizione validata.
4. Isolamento del LED rosso PA25, verifica resistenza da **1 kΩ** in serie alla base BC337 (i 220 Ω dell'utente sono sul LED IR), controllo alimentazione +5 V e GND comune, verifica del ricevitore PA26.
5. Backup completo verificato e procedura di recupero OpenBeken prima di un flash; non modificare OpenBeken 1.18.321 durante queste fasi.

**Stato:** sviluppo avanzato del protocollo e della sicurezza, **non** firmware finito. Non distribuire un file `.bin` come se fosse verificato.
