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

## Decisione architetturale confermata — 25 novembre 2026

**Obiettivo scelto esplicitamente dall'utente: la seconda opzione.** ALBATROS AI deve diventare **l'intelligenza centrale personale e domestica**, non soltanto un gestore di progetti: conversazione, memoria persistente, gestione attività e, in seguito, orchestrazione di Home Assistant tramite il satellite vocale Alexa già funzionante. La voce deve essere uno dei canali di interazione, non l'unico.

- **Gemini:** componente di ragionamento; il modello non deve poter invocare arbitrariamente servizi o dispositivi.
- **ALBATROS:** agente che media richieste, controlla autorizzazioni, mantiene memoria e seleziona strumenti consentiti.
- **Home Assistant:** sorgente degli stati e attuatore di servizi, con integrazione esplicita, minima e verificabile; inizialmente solo lettura.
- **Alexa ESP32-S3 già funzionante:** NON sostituirla o alterare ora firmware/pipeline; l'integrazione vocale si farà dopo prove isolate.
- **Home Assistant/Music Assistant/DietPi attuali:** preservarli senza interruzioni, e chiedere conferma prima di qualsiasi azione operativa.
- **Sicurezza:** credenziali solo fuori GitHub; prima sola lettura e dispositivi di test, poi comandi limitati per stanza/dispositivo con autorizzazione; escludere inizialmente serrature, allarmi e attuazioni ad alto rischio.

**Ordine aggiornato:** (1) collaudo reale chat web V1.4 da Codespaces e smartphone; (2) definizione modulo Home Assistant in sola lettura con test mock e configurazione sicura; (3) prova controllata su un'entità di test; (4) funzioni domotiche autorizzate; (5) collegamento vocale con il satellite Alexa, mantenendo il sistema già funzionante.

Questa decisione NON significa che Home Assistant o la voce siano già integrati: è il requisito di prodotto da seguire nelle prossime versioni.


## Collaudo reale V1.4 completato — 25 novembre 2026

**Verificato dall'utente nel Codespace GitHub "stunning lamp", ramo `albatros-ai-v1-3`:**
- `git status --short` pulito; `git pull --ff-only origin albatros-ai-v1-3` completato con fast-forward.
- `python -m unittest discover -s tests -v`: **29 test superati (OK)**.
- Chat web aperta tramite forwarding privato della porta 8765: messaggio «ciao» e risposta appropriata ottenuta, dimostrando l'operatività della chat e del provider Gemini.
- Richiesta «Quali progetti ho salvato nella tua memoria?» ha restituito il progetto **ALBATROS AI** con descrizione «sviluppo del mio agente intelligente personale».
- Richiesta «Quali attività ho ancora da completare nel progetto ALBATROS AI?» ha restituito «Integrare l'assistente vocale», **ID 1**, ancora da completare.
- Le verifiche precedenti sono confermate da schermate mostrate dall'utente in chat. La memoria SQLite resta nel Codespace. **La cronologia visibile delle conversazioni web non è persistente**, e il Codespace può sospendersi.
- Il software e i satelliti vocali domestici non sono stati modificati. **Home Assistant non è ancora integrato.**

**Prossimo passo concordato:** V1.5, costruire e testare un modulo Home Assistant **sola lettura**, prima con risposte mock e senza usare credenziali reali o modificare impianti, poi prova controllata; successivamente funzioni con autorizzazione e integrazione vocale.

**LUMO:** firmware acquistato in formato immagine Raspberry Pi `.img.gz`, molto grande; analisi locale futura in sola lettura, senza caricarlo su GitHub. Lo STT di LUMO è escluso dagli obiettivi; non assumere che modelli/componenti proprietari siano riutilizzabili senza verificare licenze.


## Progresso V1.5 — 25 novembre 2026 (fase 1)

- Confermato il **collaudo web reale V1.4** dal checkpoint precedente: non ripetere collegamento web.
- Aggiunto `ha_readonly.py`: modulo isolato di sola lettura, mock di default con due sensori dimostrativi; modalità `--live` solo tramite opt-in esplicito e variabili ambiente `ALBATROS_HA_URL` / `ALBATROS_HA_TOKEN`.
- Aggiunto `tests/test_ha_readonly.py` (otto test unitari: assenza rete mock, autenticazione, GET, filtraggio attributi, input, redirect); documentazione `HA_V15.md`.
- Il modulo **non è ancora inserito negli strumenti dell'LLM o nella web chat**; nessun collegamento reale a Home Assistant, nessuna credenziale fornita; nessun cambiamento a DietPi, Alexa o LUMO.
- Il modulo non invia POST/PUT/DELETE e non può chiamare servizi. Redirect di rete bloccati preventivamente.
- Ultimo commit tecnico verificato `f94144de53f1836d157aa075e72476ec466cfd46`; [GitHub Actions 45628181358](https://github.com/lorenzo2979/loreme/actions/runs/45628181358) **success**.
- **Prossimo passo:** aggiornare Codespace con `git pull --ff-only` dopo aver verificato status, lanciare `python -m unittest discover -s tests -v` e `python ha_readonly.py`. Solo dopo valutare integrazione controllata nell'agente e lettura di una entità domestica innocua.
