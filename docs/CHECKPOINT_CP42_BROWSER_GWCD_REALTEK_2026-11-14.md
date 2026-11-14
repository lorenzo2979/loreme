# Checkpoint CP42 — browser 96 tasti, GWCD/Flash autenticato e build KM4 (14/11/2026)

**STATO: software di sviluppo, NON installabile sulla GW006, NON FLASHARE.**

## Risultati ottenuti e salvati nel branch review
- Browser senza app Android: `web/gw006_remote.html`, `web/gw006_remote_ui.js`, `web/gw006_protocol.mjs`.
- Layout completo: TV36, clima14, decoder32, speaker14 = 96 tasti; pressione breve = trasmissione IR salvata, pressione lunga 3 s = menu acquisizione/prova/salvataggio/eliminazione e stato dei codici.
- Client WebCrypto GWAS v1 HMAC-SHA256 compatibile con protocollo C: CRC32 GWIR v3, nonce per avvio, contatori anti-replay non riutilizzati e nessun retry automatico dei comandi IR dopo rete incerta.
- **HTTPS obbligatorio prima di inserire chiave personale o nonce**; server Realtek attuale è HTTP, quindi i comandi restano deliberatamente disabilitati sul prodotto non provisionato. Non vengono persistite chiavi nel browser.
- Codifica GWCD v1 esattamente conforme al lettore C ufficiale: 16 byte header + 1–5 frame TX-shaped + CRC32; massimo 41.040 byte per record, 2.048 impulsi x 5.
- Transazione STORE_BEGIN / STORE_CHUNK / STORE_COMMIT / STORE_INFO con verifica della risposta firmata e dello stato del tasto.
- Il flusso di apprendimento conserva il pacchetto non ancora confermato nella coda IR; il bottone **Salva** esegue commit permanente prima di ogni RX_ACK. Scarto pacchetto solo mediante scelta esplicita. Il codice previene disconnessioni dal menu durante la transazione.
- Asset browser integrato nel modulo `gwir_dashboard_asset.c` mediante `scripts/build_gw006_browser_asset.py`, gzip deterministico, CSP con SHA256 del codice e del CSS incorporati. Sorgenti web JS/MJS sono inclusi nel manifesto SHA256 del runner dopo patch.

## Evidenze automatiche
- CI browser/host CP42: https://github.com/lorenzo2979/loreme/actions/runs/43598879393 **SUCCESS**: GWAS HMAC, CRC, replay, 96-button bitmap, TX/delete, GWCD 41.040 byte e **vera lettura del blob generato JS col parser C pinned `gwir_gwcd_store.c`**.
- CI SDK Realtek: https://github.com/lorenzo2979/loreme/actions/runs/43599011535: archive originale con checksum valido, source inventory post-patch **197 file e 45 C registrati**, asset generato **27.253 byte raw / 9.303 gzip**, test host + mock PASS, **KM0_NATIVE_BUILD=PASS, KM4_NATIVE_BUILD=PASS**.
- Il gate rigoroso di final link resta **FAIL** con gli stessi 9 moduli CP41 ancora senza sezioni allocate: `gwir_amebad_board_pins`, `gwir_apconfig_http`, `gwir_apconfig_http_stream`, `gwir_apconfig_tcp_transport`, `gwir_dashboard_sta_server`, `gwir_network_profile`, `gwir_sdk_sta_dashboard`, `gwir_stream`, `gwir_wbrg1_flash_port`. Le funzioni browser/auth/GWCD attive sono invece presenti nel link; nessun gate è stato disattivato per ottenere un falso PASS.
- Runner definitivo dopo revisione del blocco UI: https://github.com/lorenzo2979/loreme/actions/runs/43599275940. **KM0 PASS, KM4 PASS, 197 file / 45 C con source lock PASS, asset 27.439 byte raw / 9.347 byte gzip.** Il gate finale di link resta **FAIL** esattamente per i nove moduli hardware/legacy elencati sopra. Questo risultato è verificato dai log GitHub e non è un firmware installabile.

## Blocchi irrisolvibili con sola compilazione
1. **TLS sul server embedded** e provisioning sicuro/identità unica per browser senza app: il server HTTP esistente non soddisfa il requisito di non esporre chiavi Wi-Fi o HMAC a manomissione in transito. Non abilitare workaround senza cifratura.
2. **Partizione Flash reale** da almeno **4.378.624 byte** dedicati al journal NOR (96x5 caso peggiore), non verificabile su questa GW006 senza backup completo e mappa settori; `GWIR_WBRG1_STORAGE_APPROVED=0` rimane il gate.
3. **IR fisico**: conflitto possibile PA25 LED TX, RX PA26, pinmux, IRQ e driver hardware non collaudati sulla scheda.
4. APConfig da pulsante, DHCP, STA IP statico, scansione Wi-Fi e DuckDNS protetto con rollback non ancora provati nel reale flusso di boot; il token DuckDNS NON va trasmesso nell'AP aperto HTTP.
5. Mappa ROM/bootloader, PSRAM, backup/ripristino e power-cycle su device reale; OpenBeken 1.18.321 deve restare intatto.

## Regole di rilascio
- Nessun flash, nessun merge su `main` e nessun artificio nel linker per fingere che una partizione non approvata sia operativa.
- Questo checkpoint **NON** è firmware pronto per l'uso. Il prossimo lavoro reale necessita delle informazioni hardware e poi di HTTPS, APConfig e validation end-to-end.
- Il branch `main` resta al checkpoint CP19 (`5892d2dd96a50be19a65d87a385fdb5a9ef1fc97`).
- Checkpoint precedente: `docs/CHECKPOINT_CP41_NATIVE_BROWSER_HTTP_LINK_2026-11-14.md`.
