# ALBATROS AI V1.3 — prova privata Gemini/Groq

Agente personale testuale sperimentale, Python 3.11+. Mantiene progetti e attività in SQLite. Non modifica Alexa, Home Assistant o Music Assistant.

## Domattina, sul PC Windows

1. Estrai lo ZIP in una cartella privata.
2. Apri quella cartella in Esplora file. Clicca sulla barra degli indirizzi, scrivi `cmd` e premi Invio.
3. Esegui i comandi **uno alla volta**:

```bat
python --version
python -m unittest discover -s tests -v
python agent.py --self-check
python agent.py --provider gemini --api-key-prompt
```

L'ultimo comando chiede la chiave Gemini **senza mostrarla mentre la digiti** e senza inserirla nella cronologia dei comandi. Non salvare chiavi in file, screenshot, GitHub o chat. L'input nascosto può sembrare bloccato: digita la chiave e premi Invio. La chiave rimane nella memoria del processo mentre il programma è attivo.

**Nota privacy:** ogni domanda e gli eventuali risultati degli strumenti scelti dall'agente sono inviati a Gemini quando usi la modalità cloud. Non inserire segreti o dati sensibili nelle prime prove.

Prima prova: `Ciao ALBATROS, presentati in una frase.`. Seconda prova: `Quanto fa (12+3)*2?`. Terza prova: `Mostrami i progetti salvati.`. Chiudi digitando `esci`.

Se vuoi usare Groq, esegui `python agent.py --provider groq --api-key-prompt`. Non sono richiesti pacchetti Python esterni per la console di base. Servono rete e chiave API attiva per le modalità cloud. Quote e modelli dipendono dal provider e dal tuo account.

## Controlli verificati

- Test unitari locali senza richieste API reali.
- Controllo `--self-check` senza provider e senza modifiche al tuo database (usa una directory temporanea).
- Gestione di HTTP 401, 403, 404 e 429 che non mostra il corpo delle risposte o le chiavi.
- Chiave richiesta con input nascosto (`getpass`), mantenuta nella memoria della sessione.

## Limitazioni

- Non abbiamo ancora collaudato **chiamate reali** con Gemini/Groq.
- L'agente può sbagliare e le conferme proteggono solamente gli strumenti di scrittura esposti.
- Non include riconoscimento vocale, Docker o modello locale.
- La cronologia della conversazione dura soltanto nella sessione corrente; le note e i progetti SQLite restano sul disco.
- Esegui le prove in un ambiente privato e non sul DietPi di produzione.
