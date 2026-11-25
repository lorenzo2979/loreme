# ALBATROS AI V1.2 — checkpoint verificato 25/11/2026

**Progetto:** agente personale testuale, distinto dal Multi-STT e non collegato ad Alexa, Home Assistant o Music Assistant.

## Consegna verificata
- Archivio ZIP completo consegnato in chat `ALBATROS_AI_V1_2_TESTATO.zip`.
- SHA256: `b4458fbe3561735bbc62262297b3d29d53078a6c2462f2b5b9965b49a0f8202a`.
- `20 test Python` completati con esito OK, con `python -m unittest discover -s tests -v`.
- Controllo statico Python `compileall` OK.
- ZIP verificato integro con `unzip -t`.
- Demo offline eseguita con creazione progetto, creazione attività, elenco e completamento: esito verificato su SQLite.

## Nuove caratteristiche effettivamente implementate
- Cronologia di chat in RAM limitata a quattro scambi conclusi (8 messaggi); nuova sessione = cronologia vuota.
- La richiesta Gemini usa `systemInstruction` e messaggi `contents` con ruoli strutturati, con `responseMimeType: application/json`.
- Il collegamento Groq mantiene l'API compatibile OpenAI; prove HTTP solo mediante simulazione locale (mock), senza token reali.
- Le risposte degli strumenti e la memoria sono esplicitamente classificate come dati non affidabili nelle istruzioni dell'agente.
- Conferme esplicite per scritture e cancellazioni originate dal modello conservate dalla V1.1.
- SQLite persistente per memoria, progetti e attività.

## Test ancora mancanti
- Chiamate REALI a Groq e Gemini, compatibilità esatta dei modelli gratuiti e limiti API.
- Browser/GUI, speech, Docker, accesso Home Assistant, Music Assistant, cloud 24/7.
- Test approfondito di prompt injection e affidabilità del modello quando utilizza strumenti: i test mock non garantiscono l'assenza di problemi.

## Importante
Il sorgente V1.2 completo è nel pacchetto ZIP consegnato in conversazione. Questo checkpoint su GitHub **non** equivale a commit di tutti i file sorgenti V1.2. Il codice del precedente Multi-STT rimane separato. Nessuna modifica al server DietPi o al satellite Alexa.
