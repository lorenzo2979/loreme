# ALBATROS — prova locale eseguita il 24 novembre 2026

Questo documento registra l'esito **verificato**, non equivale ad annuncio di un Docker pronto.

## Esito
- Recuperato il sorgente ZIP della revisione precedente di ALBATROS Multi-STT.
- **9 test Python riusciti** (`python -m unittest discover -s tests -v`).
- Uno dei test avvia un **vero processo separato** del server e ne verifica la comunicazione Wyoming TCP (`describe/info` e `audio-stop/transcript`) senza simulare la rete.
- Testati il confronto di due trascrizioni concordi e il rifiuto di `accendi` vs `spegni`.
- Corretto il `Dockerfile` eliminando l'installazione di Speech-to-Phrase legacy 1.4.3, sostituita con immagine Python 3.11 e `faster-whisper`.
- Configurato `docker-compose.yml` **solo per prova isolata** sulla porta host `127.0.0.1:11350`; cloud OFF e un solo modello locale. Non modifica HA.
- Archivio sorgente consegnato in conversazione: `ALBATROS_PROTOTIPO_WYOMING_VERIFICATO_2026-11-24.zip`, SHA256 `ebb95927ba4b0efb2ee06635b62f86a7b93d9a42442e4562041c985feb329d6e`.

## Limitazioni confermate
- Non è stato possibile eseguire `docker build`: nessun Docker installato nell'ambiente di sviluppo.
- `faster-whisper` non è stato installabile nel container di sviluppo, a causa della mancanza di rete; **nessuna inferenza con modello Whisper è stata verificata**.
- Il Dockerfile ha soltanto il motore Whisper, connettori cloud e gateway: **Speech-to-Phrase CTC 2.x manca ancora**.
- Con `MIN_AGREEMENTS=2` e soltanto Whisper locale, la risposta sarà volutamente vuota per evitare comandi errati.
- Nessuna chiamata API reale né prove su DietPi. Non collegare a Home Assistant produttivo.

**Importante:** il pacchetto ZIP creato nel contesto della conversazione contiene le modifiche testate; questo file GitHub è il report, non la sincronizzazione del ZIP. Non dichiarare pubblicate nel repository tutte le modifiche al prototipo se non vi sono stati singoli commit.
