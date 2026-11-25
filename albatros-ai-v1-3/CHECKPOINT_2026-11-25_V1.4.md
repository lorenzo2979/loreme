# ALBATROS AI — CHECKPOINT 2026-11-25 (V1.4)

## Stato verificato

- Repository GitHub: `lorenzo2979/loreme` (pubblico).
- Ramo di lavoro: `albatros-ai-v1-3`.
- Commit verificato **prima di questo checkpoint**: `799a9cfc3b193a1a0bb420bca70f2e408246c404`.
- Workflow GitHub Actions `ALBATROS AI V1.3 - Python tests`: esito **SUCCESS**, run `45574675204`, associato al commit sopra; https://github.com/lorenzo2979/loreme/actions/runs/45574675204 .
- Codice e test della V1.3 conservati nel ramo; file web V1.4 aggiunti e pubblicati.
- **Non ancora verificato sul Codespace con browser**: avvio del server web, accesso da smartphone, autenticazione, dialogo reale web con Gemini e conferme UI. Un workflow verde **non** equivale a collaudo end-to-end.
- Non sono state apportate modifiche ad Alexa, Home Assistant, Music Assistant, Raspberry o DietPi.

## Funzioni confermate in prova reale (V1.3 da terminale)

- Gemini risponde in italiano e usa strumenti di calcolo e gestione progetti.
- SQLite conserva un progetto `ALBATROS AI`, descritto come «sviluppo del mio agente intelligente personale».
- Attività salvata e riletta: **ID 1 — «Integrare l’assistente vocale»**, **da completare**. Non marcarla conclusa prima dell'integrazione.
- Creazione progetto e attività richiedono autorizzazione esplicita.
- Il prompt `Tu>` e la risposta `ALBATROS>` sono stati testati nel Codespace anche dopo la pulizia dei file GitHub.

## Memoria / backup e privacy

- Memoria del Codespace: `/workspaces/loreme/albatros-ai-v1-3/data/albatros.sqlite3` (ultima dimensione osservata: 28 KB).
- Backup SQLite coerente creato con `sqlite3.Connection.backup` in `/tmp/albatros-backup.sqlite3`, copiato in `/workspaces/loreme/albatros-backup.sqlite3` e poi **scaricato sul PC Windows**, confermato da screenshot in chat.
- Il database **non è stato caricato su GitHub**. La protezione `.gitignore` è stata salvata e pubblicata con commit `ad08b51` prima della V1.4; contiene `albatros-ai-v1-3/data/`, `albatros-backup.sqlite3`, `*.sqlite3`, `.env`.
- Il backup sul PC è una fotografia di quel momento, **non** un backup automatico dei futuri cambiamenti. Non usare `git add .` indiscriminatamente. Non pubblicare chiavi API, password o database.
- La cartella `data/` e il backup possono essere persi eliminando il Codespace; tenerne una copia esterna privata.

## V1.4 — file aggiunti (implementazione pubblicata, da collaudare)

- `albatros-ai-v1-3/web_app.py`: server HTTP Python, accesso con codice temporaneo e sessione, API chat/approvazione, riuso di `agent.ask` e della stessa memoria SQLite.
- `albatros-ai-v1-3/web/index.html`: pagina chat responsive.
- `albatros-ai-v1-3/web/style.css` e `web/app.js`: interfaccia smartphone, invio messaggi, visualizzazione delle conferme Autorizza/Rifiuta.
- `albatros-ai-v1-3/tests/test_web.py`: test offline del flusso di risposta e conferma simulato.
- `albatros-ai-v1-3/WEB_V14.md`: guida d'avvio.
- Nessuna nuova libreria Python esterna richiesta dal server web.
- Il server è progettato per ascoltare sulla porta **8765**, su `127.0.0.1`; il port forwarding Codespaces deve rimanere **PRIVATE**, mai pubblico. L'accesso da telefono richiede Codespaces funzionante e sessione GitHub autorizzata.
- Il provider Gemini richiede una chiave inserita nel prompt **nascosto**, mai da condividere.
- Questa versione non è autonoma 24/7 e non offre ancora controllo vocale, Alexa o Home Assistant.

## Cronologia della pulizia GitHub

- Rimossi i vecchi file GW006 e Android dai quattro rami ALBATROS allora presenti, mantenendo il codice ALBATROS.
- Il ramo di lavoro `albatros-ai-v1-3` era stato cancellato durante la pulizia e **ricreato da `main`**.
- Nel Codespace era rimasta una cronologia Git più vecchia: backup locale `backup-albatros-prima-allineamento`, `git fetch`, riallineamento a `origin/albatros-ai-v1-3`, ripristino di `.gitignore` e push riuscito `ad08b51`.
- All'ultima verifica esistono i rami: `main`, `albatros-ai-v1-3`, `review/gw006-cp23-native-probe`, `review/gw006-cp30-native-source`, `review/gw006-cp31-owner`. **Tre rami storici GW006** restano presenti: non necessari per ALBATROS. Non eliminare `main` o `albatros-ai-v1-3`.
- Gli esperimenti Multi-STT e Speech/Music non sono prodotti finiti. Il codice Multi-STT è ancora presente come cartella nel ramo di lavoro; i rami sperimentali precedenti non risultano più presenti nell'ultima lista dei rami.

## Ripresa operativa (uno step alla volta)

1. Aprire il Codespace GitHub di `lorenzo2979/loreme` nel ramo `albatros-ai-v1-3`. **Non eliminare il Codespace**.
2. In un terminale Linux separato dal prompt `Tu>`, eseguire:

   ```bash
   cd /workspaces/loreme
   git status --short
   git pull --ff-only origin albatros-ai-v1-3
   cd albatros-ai-v1-3
   python -m unittest discover -s tests -v
   python web_app.py --provider gemini --api-key-prompt
   ```

3. Se `git pull` fallisce o mostra divergenze, **fermarsi e ispezionare lo stato**. Non usare `git reset --hard`, `git clean` o `git push --force` per tentativi.
4. Nel terminale digitare la chiave Gemini quando richiesto, senza mostrarla né divulgarla; conservare privatamente il codice temporaneo di accesso.
5. Aprire la scheda **Ports** del Codespace, porta **8765**, visibilità **PRIVATE**; aprire in browser e verificare la chat.
6. Prove successive: inviare «ciao»; leggere il progetto e l'attività ID 1; verificare il rifiuto di un'operazione di prova prima di consentire modifiche reali. Non completare l'attività vocale.
7. Solo dopo il collaudo web, pianificare backup SQLite periodico su destinazione privata, assistente vocale e collegamento Home Assistant in sola lettura.

## Vincoli di progetto

- Procedere con passi semplici e verifiche reali. L'utente segue anche da smartphone.
- Non richiedere chiavi API in chat e non committare segreti.
- Conservare il sistema vocale Alexa e Home Assistant già funzionanti; non introdurre modifiche al sistema domestico senza prove e consenso.
- Non confondere test unitari verdi con una prova reale del server web.

Checkpoint redatto il **25 novembre 2026**.
