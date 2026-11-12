# GW006 IR Universal — CP-17 (12 novembre 2026)

**PROGETTO SORGENTE, NON FLASHARE. OpenBeken 1.18.321 resta intatto.**

## Codice nuovo
- `firmware/core/gwir_gwcd_store.c/.h`: lettore streaming, **sola lettura**, del formato file GWCD v1 già scritto da `AndroidCodeStore.kt` (96 identità, massimo 5 frame/tasto, 2048 impulsi/frame, endian big, CRC32). Memoria di lavoro 128 byte per scansione. Rifiuta CRC errato, troncamenti, impulsi fuori limite e I/O fallite. Non accede alla flash reale né all'IR.
- Test `tests/test_gwcd_store.c`: massimo 5×2048, corruzione CRC, errori callback e limiti PASS.
- Test incrociato **Android Kotlin → decoder C**: file realmente generato dal serializer AndroidCodeStore con cinque frame, letto e verificato nel C PASS.
- Integratore SDK simulato aggiornato da 33 a **34 sorgenti** e test apply/idempotence/rollback PASS; `make test` ora include il nuovo test C.
- Sanitizzatori ASan/UBSan sul nuovo modulo: PASS.

## Capienza da verificare
Massimo grezzo 96 × (20 + 5 × (12 + 2048×4)) = **3.939.840 byte**. La scheda tecnica [Tuya WBRG1](https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz) dichiara **8 MB flash, 4 MB PSRAM**, ma NON conosciamo la mappa flash né lo spazio libero del dispositivo. Allineamento a settori da 4 KiB di un record massimo = 11 settori, 96 record = 4.325.376 byte; con doppia copia completa = 8.650.752 byte, oltre gli 8 MiB nominali: progettare strategia transazionale e verificare spazio prima di promettere memorizzazione completa.

## Test e limiti
- Esecuzione completa ricostruita interrotta per limite temporale durante il test timeout fail-closed lwIP; test specifico rieseguito separatamente **PASS**, regressione selezionata conclusa con exit 0.
- `make test test-web-assets test-dashboard-lwip-abi arm-smoke test-gwcd-store` PASS nel controllo conclusivo, oltre a Kotlin→C e SDK mock. **Non è una build nativa SDK KM0/KM4 né collaudo su GW006.**
- Archivio CP-17 creato nella chat: `GW006_IR_UNIVERSAL_CP17_GWCD_READER_SOURCE.zip` con **191 file**, SHA256 `c6c04360174d869b822508eeecd1e8f1cf214e67e63ec2182ab6e07ac7960b31`.
- **I sorgenti completi dell'archivio CP-17 NON sono sincronizzati integralmente con questo repository.** Le GitHub Actions native REV21 rimangono in coda a 0 jobs alla verifica corrente.

## Passi bloccanti
1. Sincronizzazione effettiva sorgenti CP-17 nel repository SDK runner; build/link **nativa** KM0/KM4 Realtek con log.
2. Mappa flash reale: backend nonvolatile con allocazione, rollback/power loss e budget; ora esiste solo decoder readonly.
3. API browser IR HMAC autenticate, integrazione web→motore IR e controllo temporaneo APConfig.
4. Verifica fisica PA25 (LED rosso da isolare) TX / PA26 RX, driver BC337 con 1 kΩ sulla base e ripristino flash prima di installazione.

**Non installare alcun firmware da questo checkpoint.**