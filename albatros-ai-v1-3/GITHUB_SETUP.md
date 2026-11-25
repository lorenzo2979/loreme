# ALBATROS AI V1.3 — GitHub setup

Questo ramo è dedicato all'agente personale ALBATROS AI V1.3.
È stato creato separatamente dal router Multi-STT e dal ramo main.

## Codice verificato nel pacchetto
Pacchetto locale: ALBATROS_AI_V1_3_PROVA_GEMINI.zip (SHA256 cf8140131f36cf7c823cb405e2f8774e3da65d731fa1be4f0dbdc82522d147ea).

I sorgenti sono organizzati in una cartella albatros-ai-v1-3:
- agent.py
- README.md
- .gitignore
- tests/test_agent.py
- tests/test_projects.py
- tests/test_v12.py
- tests/test_v13.py

**Attenzione:** questo file da solo non pubblica i sorgenti nel repository. Occorre ancora importarli nel percorso albatros-ai-v1-3.

## Test automatizzati
GitHub Actions: .github/workflows/albatros-ai-v13.yml.
Quando i file sorgenti saranno presenti, il workflow potrà controllare
sintassi, test Python e self-check senza usare credenziali cloud.
Il workflow non deve salvare il database utente nel repository.

## Protezione delle API
Non caricare file .env, chiavi Gemini/Groq, token o database personali.
Per le future prove con API reali utilizzare solo segreti privati del repository
e workflow manuali dopo aver esaminato i permessi.

## Cosa non fa il sistema
Nessun controllo Alexa / Home Assistant / Music Assistant;
nessuna conversazione cloud reale ancora verificata.
