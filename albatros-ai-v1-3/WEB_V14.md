# ALBATROS AI V1.4 — guida web privata

Questa versione aggiunge una chat adatta al telefono, senza dipendenze Python esterne. Riutilizza l'agente Gemini V1.3, le autorizzazioni e il database SQLite esistente.

## Avvio in GitHub Codespaces

1. Apri il Codespace nel ramo albatros-ai-v1-3.
2. Apri un nuovo terminale (non il prompt Tu> dell'agente).
3. Esegui i seguenti comandi UNO ALLA VOLTA:
   git status --short
   git pull --ff-only origin albatros-ai-v1-3
   cd albatros-ai-v1-3
   python -m unittest discover -s tests -v
   python web_app.py --provider gemini --api-key-prompt
4. Inserisci la chiave Gemini nel prompt nascosto. Non comunicarla in chat o screenshot.
5. Conserva privatamente il codice di accesso temporaneo stampato nel terminale.
6. Nella scheda Ports di Codespaces, apri la porta 8765. Imposta la visibilità PRIVATE, mai Public.
7. Apri il link privato dal PC oppure, se il forwarding lo consente, dal telefono autenticato a GitHub.
8. Inserisci il codice temporaneo e parla con ALBATROS. Le scritture mostrano Autorizza/Rifiuta.

Se il pull fallisce, non usare reset --hard, clean o push --force; verifica prima i file.

## Sicurezza e limiti

La porta deve restare PRIVATE e il server si lega a 127.0.0.1. Un codice temporaneo aggiunge un secondo livello di accesso. La memoria e la chiave API non vanno caricate su GitHub. Non comunicare codici o chiavi.
Il Codespace può sospendersi; la chat non funziona se il server non è avviato. La cronologia della chat non persiste, mentre le attività SQLite restano nel database locale. Conserva il backup sul PC.
L'agente non controlla ancora Alexa, Home Assistant o Music Assistant.
Per arrestare la chat premi Ctrl+C nel terminale del server.
