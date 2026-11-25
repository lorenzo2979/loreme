# ALBATROS AI V1.3 — checkpoint, 25 novembre 2026

**Obiettivo:** test Gemini e Groq più sicuro, senza modificare Alexa, Home Assistant o DietPi.

## Risultati locali verificati
- Codice V1.2 recuperato dall'archivio locale, esaminato e revisionato.
- **25 test Python superati** (20 precedenti + 5 nuovi test sugli errori delle API).
- `python agent.py --self-check`: risultato positivo su strumenti e SQLite temporaneo, senza richiedere chiavi cloud.
- Archivio ZIP testato integro: `ALBATROS_AI_V1_3_PROVA_GEMINI.zip`.
- SHA256 dell'archivio: `cf8140131f36cf7c823cb405e2f8774e3da65d731fa1be4f0dbdc82522d147ea`.

## Funzioni aggiunte
- `python agent.py --provider gemini --api-key-prompt`: chiede la chiave Gemini tramite input nascosto (`getpass`) anziché richiedere di digitarla nel comando Windows; chiave non salvata dal codice su disco.
- Opzione Groq analoga.
- Messaggi diagnostici per errori HTTP 400/401/403/404/429/500/503 e timeout senza mostrare il corpo della risposta HTTP o la chiave.
- `--self-check` non modifica il database utente.

## Cosa NON è stato verificato
- Nessuna chiamata **reale** a Gemini o Groq: serve una chiave privata e la quota disponibile nel suo account.
- Nessun modello addestrato o eseguito nel cloud da noi.
- Nessuna integrazione voce/Home Assistant, nessuna build Docker.

**Importante:** i sorgenti completi di questa revisione sono nel file ZIP consegnato in questa chat; questo checkpoint GitHub da solo non costituisce un commit completo dei sorgenti V1.3.

## Prova Windows, nella cartella estratta
```cmd
python -m unittest discover -s tests -v
python agent.py --self-check
python agent.py --provider gemini --api-key-prompt
```

Non condividere la chiave API con l'assistente e non caricarla in repository pubblici.
