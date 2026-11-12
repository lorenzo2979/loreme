# GW006 IR Universal — Checkpoint CP-15 (12 novembre 2026)

**Target:** GW006 / WBRG1 / RTL8721CSM. **Stato: sorgenti CP-15 testati solo su host, NON firmware flashabile.** Conservare OpenBeken 1.18.321.

## Novità di CP-15
- `firmware/core/gwir_sta_gate.c/.h`: gate che richiede modalità STA esclusiva (nega AP e STA+AP), associazione e link attivi, netif UP, IP valido e invariato.
- `firmware/realtek/gwir_dashboard_sta_server.c`: ripete il controllo di stato prima di ricevere e di trasmettere ogni blocco HTTP. Se la rete passa ad APConfig durante una richiesta, il trasferimento termina.
- `firmware/sdk/gwir_sdk_sta_dashboard.c/.h`: task FreeRTOS **sperimentale**, basato sulle API vendor `wifi_mode`, `wifi_is_connected_to_ap`, `xnetif[0]`, `netif_is_up`, `netif_is_link_up`, `LwIP_GetIP`. Intende pubblicare la dashboard read-only su **STA:8080**; non è ancora eseguito su hardware.
- **Avvio DISABILITATO per default**: solo `GWIR_ENABLE_STA_DASHBOARD_TRIAL=1` abilita la creazione del task. Nessuna modifica ai GPIO.
- Aggiornato `scripts/integrate_amebad_sdk.py` per includere **33 sorgenti C**, compresi asset dashboard, HTTP e gate.

## Test e limiti
**PASS su Linux:** regressione C/Kotlin, autenticazione e IR, HTTP socket TCP reale locale, test di passaggio STA→AP durante richiesta incompleta, 33 oggetti SDK **mock** compilati con rollback/dry-run, unit test gate, compilazione del task contro header simulati, sanitizer ASan/UBSan e ARM compile-only.

**Non è stata effettuata una build nativa SDK KM0/KM4 della CP-15 né il flash/test sulla GW006.** Gli header di test SDK non sono il firmware vendor reale. L'interfaccia HTML rimane di sola lettura e non trasmette IR. La verifica di stato della rete non garantisce l'isolamento AP/STA in tutti i casi finché non si collauda sul dispositivo.

## Componenti mancanti
1. Realtek SDK KM0/KM4: build e link **nativi**, inizializzazione del task e test della scheda in modalità STA e APConfig.
2. Memoria permanente chip per 96 tasti × fino a 5 pacchetti IR, con validazione flash/corruzione/perdita alimentazione.
3. API web autenticate HMAC-SHA256 + anti-replay / accesso IR e integrazione interfaccia.
4. Collaudo hardware TX **PA25** (occupato LED rosso da isolare), RX **PA26**, driver BC337 5V con **1 kΩ base**, 220 Ω LED e massa comune.
5. Backup flash originale e ripristino HW provati **prima di qualsiasi flash**.

## Archivio sorgente
Archivio creato in questa chat `GW006_IR_UNIVERSAL_CP15_STA_FREERTOS_SOURCE.zip` (180 file); SHA-256 `bc62f6f840d07d669209c6a55c843b6a1318b6b906061875e6f083d645b99a69`.

**Nota:** questa pagina GitHub è il checkpoint; i sorgenti CP-15 completi sono nell'archivio della chat e **non sono automaticamente sincronizzati nel repository**.