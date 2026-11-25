# ALBATROS — Proof of Concept reale e blocco individuato (24-11-2026)

## Fatti verificati nelle sorgenti ufficiali

- **Speech-to-Phrase moderno:** https://github.com/OHF-Voice/apps/blob/main/speech-to-phrase/DOCS.md
  * Supporta l'italiano `it`, grammatica vincolata, un confidence gate `Max score`, frasi personalizzate in UI, Wyoming.
  * Il sistema documentato legge dall'entity registry nomi/aree di Home Assistant e trigger di automazione; questi due ultimi tipi di sorgenti possono essere disattivati in UI, ma il deployment documentato resta un'app HA. Non è stato verificato un avvio headless con zero connessioni al sistema HA.
  * NON usare il suo servizio HA come backend ALBATROS: violerebbe il requisito.
- **Speech-to-Phrase legacy:** https://github.com/OHF-Voice/speech-to-phrase
  * L'immagine `rhasspy/wyoming-speech-to-phrase` documentata necessita `--hass-websocket-uri` e `--hass-token` per assemblare grammatica da HA. Non è autonoma così com'è.
- **Rhasspy Speech:** https://github.com/rhasspy/rhasspy-speech
  * MIT, modello italiano, grammatica personalizzabile, doppio decoder Kaldi (vincolato/meno vincolato), tools precompilati. **Archivio read-only dal 06/10/2025**.
- **Wyoming Rhasspy Speech:** https://github.com/rhasspy/wyoming-rhasspy-speech
  * Wyoming server ufficiale con immagine Docker già pubblicata. Fonte documenta il test isolato:
    `docker run --rm -it -p 127.0.0.1:11300:10300 rhasspy/wyoming-rhasspy-speech`
  * Questo comando usa una porta *di test* e NON modifica Home Assistant; richiede comunque verifica modelli/lingua configurati prima di considerarne valido il riconoscimento italiano. **È solo un esperimento**, NON il Docker finale ALBATROS.
- **Segnalazione reale docker STP:** https://github.com/OHF-Voice/speech-to-phrase/issues/59
  * Un utente Docker documenta nel 2025 che la generazione di grammatica e il dispatch intent in HA sono due problemi diversi; la configurazione STP può funzionare ma HA non interpretare un comando.

## Prossima prova adatta ad ALBATROS
1. **Prototipo isolato su una porta diversa** dal servizio attuale, senza toccare Assist e senza pubblicare token.
2. Esaminare entrypoint del nuovo Speech-to-Phrase CTC ed estrarre train/compile da configurazione di sole frasi locali (proof-of-concept senza websocket HA).
3. Se possibile, eseguire in un solo container un child-process CTC con loopback Wyoming + Faster-Whisper nella stessa immagine; il router deve restare l'unico servizio esposto.
4. Test audio italiano real-world, fallback e confronto con parole opposte e artisti; nessun comando HA durante il test.
5. Solo se la prova 2 risulta tecnicamente impossibile proporre Rhasspy Speech MIT come sostituzione, richiedendo scelta esplicita dell'utente.

## Verifica di onestà
Il codice del ramo contiene `wyoming_server.py` ed `engine_runner.py`, non ancora un'immagine integrata avviabile. Non dichiarare né container funzionante né Speech-to-Phrase autonomo finché non esistono log di build e test audio italiani. Non testare direttamente sul sistema in esercizio.


## Correzione della decisione — richiesta utente: solo soluzioni mantenute nel 2026

**Rhasspy Speech archiviato dal 06/10/2025: escluso dal percorso primario e dalla prossima prova.** Il precedente comando Docker rimane nell'archivio documentale ma non è un'azione consigliata: è una prova del passato e non va eseguito su DietPi per ALBATROS.

### Fatti nuovi verificati il 24 novembre 2026
- `OHF-Voice/apps` contiene il nuovo Speech-to-Phrase CTC che sostituisce il vecchio motore Kaldi. Italiano supportato, ma NON è ancora dimostrata una immagine standalone configurabile senza HA.
- GitHub Issue ufficiale `OHF-Voice/apps#35` (03–16 novembre 2026): utente segnala che il tag DockerHub standalone Speech-to-Phrase più recente individuato è **1.4.5**, mentre lo sviluppo in repository è oltre **2.0**; nessuna risposta nel thread indica un'immagine standalone nuova e collaudata. La discussione suggerisce Voice Incubator, che non soddisfa automaticamente i vincoli ALBATROS.
  https://github.com/OHF-Voice/apps/issues/35
- `localai-org/italian-asr` (2026): famiglia di modelli Whisper ottimizzati per italiano, **CTranslate2 INT8** pronti per `faster-whisper`, con esempi Python. Per CPU DietPi selezionare il modello in base a RAM e misurazioni, non scegliere automaticamente large-v3-turbo.
  https://github.com/localai-org/italian-asr
- `wyoming-faster-whisper` streaming: giugno 2026 un utente pubblica fork con streaming e riferisce miglioramenti N100; il manutentore upstream conferma successivo merge nella **v3.2.0**, ma i modelli sherpa streaming citati sono EN/DE: **non** assumere disponibilità streaming in italiano. 
  https://www.reddit.com/r/homeassistant/comments/1u3rzxd/

### Nuova priorità operativa
1. Non integrare software abbandonato e non spacciare Speech-to-Phrase 1.4.5 per la versione 2.x.
2. Sviluppare e testare concretamente Wyoming + Whisper locale mantenuto nel 2026 + servizi cloud a opt-in, con un singolo container; **non** abilitare nel sistema reale finché il requisito STP non è soddisfatto.
3. Studiare codice nuovo CTC e packaging autonomo senza credenziali HA. Segnare come **bloccato** fino a proof-of-concept verificata.
4. Nessun intervento su Alexa/Music Assistant/DietPi.
