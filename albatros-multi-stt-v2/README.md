# ALBATROS Multi-STT AI Agent — V2 sperimentale

**Obiettivo:** un solo container Docker su DietPi, una sola porta STT Wyoming verso Home Assistant, quattro STT gestiti *internamente* dal router: Speech-to-Phrase (principale), Whisper locale, GroqCloud, Google Gemini Audio. Un agente centrale restituisce una sola frase, oppure una stringa vuota quando non riesce a validarla.

**NON installare ancora su DietPi:** il container all-in-one non è stato costruito e collaudato con Speech-to-Phrase + Whisper realmente avviati.

## File implementati in questo ramo

- `agent.py`: consenso conservativo e protezioni per azioni opposte / numeri.
- `wyoming_server.py`: protocollo Wyoming, ingresso audio e una sola trascrizione.
- `engine_runner.py`: chiamate in cascata ai quattro provider, cloud disattivato per impostazione predefinita.
- `tests/`: unit test della valutazione, protocollo e motori simulati.
- `.github/workflows/`: test automatici nel ramo sperimentale.

Le librerie e i modelli di Speech-to-Phrase non sono ancora pronti nell'immagine Docker. Il `Dockerfile` rimane intenzionalmente un **segnaposto non installabile**. Non confondere il superamento dei test Python con un sistema vocale end-to-end funzionante.

## Schema obbligatorio

Alexa -> trasporto audio Assist -> router Wyoming (unico servizio STT) -> Speech-to-Phrase interno -> Whisper locale -> Groq -> Gemini -> agente valutatore -> una sola trascrizione per Home Assistant Assist.

Il router **non legge** entità, dispositivi, librerie o impostazioni di Home Assistant e non esegue direttamente azioni. Il modello Speech-to-Phrase richiederà frasi e nomi definiti in un file locale (non addestramento via token Home Assistant). Groq e Gemini sono servizi remoti, se abilitati inoltrano audio ai rispettivi fornitori.

## Test

```bash
python -m unittest discover -s tests -v
```

## Protezioni

- Nessun token in GitHub; le API key Groq/Gemini solo nel file privato `.env` di DietPi.
- Nessuna chiamata cloud per impostazione predefinita; configurazione esplicita richiesta.
- Nessuna modifica alla pipeline Alexa in esercizio fino al collaudo separato.
- Nessuna trascrizione inviata se manca concordanza sufficiente.
- Il protocollo Wyoming non ha autenticazione nativa: esporre il servizio solo su LAN protetta.

## Ancora da fare

1. Packaging reale di Speech-to-Phrase (nuovo progetto OHF-Voice/apps o implementazione legacy con tool nativi) e modello italiano con frasi locali, senza dipendere da HA.
2. Build Docker verificata con entrambi i motori locali, nel medesimo container.
3. Test con registrazioni italiane reali, compresi nomi di artisti, verbi contrari, VAD e tempi massimi.
4. Chiamate reali Groq/Gemini con limiti gratuiti e autorizzazione cloud.
5. Integrazione Wyoming in Home Assistant *solo dopo* i test standalone.

Riferimenti tecnici: https://github.com/OHF-Voice/apps , https://github.com/OHF-Voice/speech-to-phrase , https://github.com/OHF-Voice/wyoming
