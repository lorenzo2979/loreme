# CHECKPOINT CP41 — GW006 browser HTTP nel KM4 reale (14/11/2026)

**STATO: PROGRAMMA IN SVILUPPO, NON INSTALLABILE, NON FLASHARE SU GW006.**

## Risultato concreto
Il codice su `review/gw006-realtek-flash-hal` ora contiene un unico server STA autenticato per le richieste TCP GWAS e l'ingresso HTTP del browser:
- `firmware/core/gwir_web_rpc.c/.h`: valida una busta GWAS v1 singola e invoca l'owner autenticato esistente; nessun reset dei contatori tra richieste.
- `firmware/core/gwir_web_http_post.c/.h`: parser rigoroso per `POST /api/gwir`, Host e Origin locali esatti, Content-Length univoco, nessun chunked o credenziale HTTP alternativa.
- `firmware/realtek/gwir_amebad_auth_server.c`: il **medesimo task seriale** gestisce richieste HTTP e flusso GWAS firmato; la lettura della pagina GET è statica, senza accesso IR non autenticato. Tutti i comandi che cambiano stato passano per GWAS firmato e il runtime owner; non esistono credenziali compilate nel sorgente.
- L'interfaccia statica attuale non implementa ancora i 96 tasti WebCrypto. Il suo CSP blocca le chiamate fetch: non dichiararla dashboard operativa.
- Il server viene raggiunto soltanto dalla catena di boot esistente **se** è presente una configurazione GW006 con credenziali/nonce board-specific approvate. Nei build di sviluppo senza provider, il server NON si avvia.

## Prove registrate
- Host parser + bridge + compilazione del socket server: **SUCCESS** https://github.com/lorenzo2979/loreme/actions/runs/43591568538
- Build nativa ufficiale Realtek AmebaD pinned SHA `7569200fb8304daa61a9f09ccf439ada38d6de2e`: workflow https://github.com/lorenzo2979/loreme/actions/runs/43591987491
  - SHA locked archive original: PASS.
  - Source gate post-patch: **191 file, 45 sorgenti C / 45 registrati nel Makefile**, PASS.
  - Mock SDK: 46 oggetti incl. main, PASS; test host Flash NOR, identity e boot, PASS.
  - **KM0_NATIVE_BUILD=PASS** e **KM4_NATIVE_BUILD=PASS**.
  - Final-link strict gate: **FAIL**, di proposito: non dichiara release pronta in assenza di componenti realmente richiamati.
  - Prima di CP41 (run 43583624928): **11** moduli esclusi. In CP41: **9** moduli esclusi. I moduli HTTP `gwir_dashboard_http` e `gwir_dashboard_asset` sono entrati nell'ELF finale tramite il percorso reale del server.
  - Sorgenti nuovi di POST GWAS sono registrati nell'SDK nativo e non risultano tra i moduli esclusi dal linker.
  - Artifact di sola ispezione, **NON FLASH**: https://github.com/lorenzo2979/loreme/actions/runs/43591987491/artifacts/14112229439

## Rimangono 9 moduli non allocati
`gwir_amebad_board_pins`, `gwir_apconfig_http`, `gwir_apconfig_http_stream`, `gwir_apconfig_tcp_transport`, `gwir_dashboard_sta_server`, `gwir_network_profile`, `gwir_sdk_sta_dashboard`, `gwir_stream`, `gwir_wbrg1_flash_port`.

**Classificazione:**
- `gwir_sdk_sta_dashboard` e `gwir_dashboard_sta_server`: vecchio server HTTP sperimentale parallelo, da ritirare dal target prodotto dopo confronto dei test con la nuova via unificata; NON avviarlo in parallelo.
- `gwir_stream`: vecchio trasporto GWIR non autenticato, non esporre; da togliere dal prodotto se non necessario.
- `gwir_apconfig_http`, `gwir_apconfig_http_stream`, `gwir_apconfig_tcp_transport`, `gwir_network_profile`: provisioning/APConfig non ancora collegato al pulsante e al ciclo Wi-Fi reale.
- `gwir_amebad_board_pins`: pin PA25/PA26 da provare fisicamente, possibile conflitto LED.
- `gwir_wbrg1_flash_port`: **intenzionalmente non abilitabile** senza mappa Flash verificata della GW006. `GWIR_WBRG1_STORAGE_APPROVED=0` rimane obbligatorio.

## Perché il firmware NON è installabile
La regione persistente necessaria per tutti i 96 tasti × fino a 5 pacchetti ha un caso peggiore di **4.378.624 byte**; l'audit di quattro dump pubblici NON individua una partizione sicura sulla GW006 dell'utente. Mancano backup/ripristino reali, test bootloader/layout GW006, test PSRAM, autenticazione con chiave/nonce hardware, APConfig completo (SSID/DHCP/statico/DuckDNS protetto), gestione IR fisica e vera UI browser 96 tasti.

Non utilizzare le immagini sperimentali native come firmware definitivo. **OpenBeken 1.18.321 e `main` rimangono intatti.**

## Prossimi passi rigorosi
1. Eliminare dal target firmware le vecchie alternative non utilizzate (TCP plain e dashboard trial), senza allentare i gate degli altri moduli.
2. Collegare APConfig e profilo rete al pulsante fisico e al provider hardware, con test fail-closed; mai trasmettere il token DuckDNS su AP aperto.
3. Realizzare la UI 96 tasti WebCrypto e il firmware solo quando provisioning reale è verificabile.
4. Approvare flash/pin/PSRAM solo da dati della **scheda GW006 specifica** e con piano di recupero collaudato; poi ripetere link, boot, IR, 96×5 persistenza e power cycle.

Riferimento checkpoint precedente: `docs/CHECKPOINT_CP40_BROWSER_AUTH_BRIDGE_2026-11-14.md`.
