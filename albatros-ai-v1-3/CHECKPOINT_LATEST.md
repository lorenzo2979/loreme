# ALBATROS AI — CHECKPOINT UFFICIALE (ULTIMO)

> **File di riferimento permanente**. Aprire sempre questo file quando si riprende ALBATROS AI da una nuova conversazione. Aggiornarlo a ogni checkpoint, senza modificarne il percorso, e conservare i checkpoint datati come archivio storico.

## Checkpoint n. 1 — 25 novembre 2026

- **Progetto:** ALBATROS AI, agente personale in italiano.
- **Repository:** https://github.com/lorenzo2979/loreme
- **Ramo di sviluppo:** `albatros-ai-v1-3` (conservare anche `main`).
- **Checkpoint storico dettagliato:** [CHECKPOINT_2026-11-25_V1.4.md](CHECKPOINT_2026-11-25_V1.4.md).
- **Ultimo commit tecnico verificato prima del primo checkpoint:** `9f8aea21c23ec2fefb04b34e959df1aec590cf99`.
- **Ultima GitHub Actions verificata:** [run 45575729802](https://github.com/lorenzo2979/loreme/actions/runs/45575729802) — **success**, test automatici del repository sul commit indicato.

### Stato certo

- **V1.3 testuale:** verificata con Gemini in Codespaces, memoria SQLite, lista progetti e attività; azioni scrivibili autorizzate dall'utente.
- **V1.4 web:** implementata e pubblicata su GitHub. Comprende `web_app.py`, `web/index.html`, `web/style.css`, `web/app.js` e `tests/test_web.py`. **Non ancora provata realmente con browser e smartphone**: non presentarla come già operativa.
- **SQLite:** database locale nel Codespace, percorso `/workspaces/loreme/albatros-ai-v1-3/data/albatros.sqlite3`. Progetto ALBATROS AI e attività ID 1 «Integrare l’assistente vocale», ancora da completare; accertati nell'ultima prova da terminale.
- **Backup:** creato tramite SQLite online backup e scaricato sul PC dell'utente. Non automaticamente aggiornato.
- **Privacy:** il file `.gitignore` esclude `albatros-ai-v1-3/data/`, `albatros-backup.sqlite3`, `*.sqlite3` e `.env`. **Non caricare database e API key nel repository pubblico**.
- **GW006:** codice vecchio e Android rimossi dai rami ALBATROS. Alla verifica precedente restavano tre rami storici GW006 separati.
- **Integrazione vocale/Home Assistant:** non ancora realizzata nell'agente AI. Non modificare l'attuale satellite Alexa o il sistema domestico funzionante.

### Prossimo passo operativo prioritario

**Collaudare V1.4 da GitHub Codespaces senza perdere il database.** Aprire un secondo terminale di Codespaces ed eseguire, uno alla volta:

```bash
cd /workspaces/loreme
git status --short
git pull --ff-only origin albatros-ai-v1-3
cd albatros-ai-v1-3
python -m unittest discover -s tests -v
python web_app.py --provider gemini --api-key-prompt
```

Se `git pull` segnala divergenze, fermarsi e controllare: **non usare automaticamente reset/clean/force push**. Inserire la chiave Gemini solo nel prompt nascosto, non in chat. In Codespaces aprire la porta `8765` dalla scheda **Ports** mantenendo visibilità **Private**. Aprire la chat, provare «ciao», la lettura delle attività e un'operazione da rifiutare senza effetti. Solo dopo verificare accesso da smartphone.

### Regole per tutte le prossime conversazioni

1. Leggere questo `CHECKPOINT_LATEST.md` e il checkpoint storico collegato **prima di modificare il codice**.
2. Controllare GitHub per commit, test e rami aggiornati; il checkpoint può essere più vecchio del repository.
3. Procedere **un passo alla volta**, distinguendo quanto è implementato, quanto è testato e quanto non è ancora verificato.
4. Dopo ogni «Fai checkpoint», **aggiornare questo stesso file** e creare un nuovo checkpoint datato solo quando utile. Non pubblicare segreti o dati personali del database.
5. Consentire all'utente di riprendere il lavoro anche da smartphone, mantenendo la configurazione domestica stabile.

**Messaggio da usare in una nuova chat:** «Riprendiamo ALBATROS AI: leggi `albatros-ai-v1-3/CHECKPOINT_LATEST.md` dal ramo `albatros-ai-v1-3` del repository GitHub `lorenzo2979/loreme` e continua dal prossimo passo.»
