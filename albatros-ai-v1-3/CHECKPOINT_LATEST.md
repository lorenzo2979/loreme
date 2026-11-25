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


## V1.5 fase 2 — integrazione Gemini/Home Assistant predisposta (25 novembre 2026)

- **Decisione utente:** continuare lo sviluppo; **collaudare tutto successivamente**, evitando prove manuali continue in Codespaces.
- In `agent.py` sono esposti due strumenti allowlist `ha_get_state` e `ha_list_states`, accessibili anche dalla chat web esistente tramite `ask()`.
- Default `ALBATROS_HA_MODE=mock`: solo due sensori simulati, nessuna richiesta HTTP; risultati marcati `source: mock` e istruzioni Gemini di dichiarare che sono simulati.
- Modalità `live` solo esplicitamente: variabili ambiente URL/token e `ALBATROS_HA_ENTITIES` con massimo 30 entità consentite. Letture GET per ID singolo; niente servizi di attuazione, niente inventario indiscriminato.
- Creati test `tests/test_ha_agent.py` per default mock, strumenti negati, restrizioni di argomenti, allowlist e ciclo Gemini simulato.
- Aggiornata documentazione `HA_V15.md`.
- **Non dichiarare collaudata end-to-end questa fase**: il codice è sul ramo GitHub, ma l'utente ha rinviato tutte le prove manuali. Nessuna credenziale reale o modifica a Home Assistant, Alexa, LUMO, DietPi.
- **Prossime attività di sviluppo senza prove manuali dell'utente:** rifinire limiti delle risposte, robustezza ed eventuale vista di stato nell'interfaccia, conservare memoria e credenziali fuori GitHub; collaudo integrato quando l'utente lo richiederà.


## Priorità aggiornata: ALBATROS Speech+ italiano — 25 novembre 2026

**Decisione utente:** la massima priorità non è aggiungere altre capacità AI, ma rendere affidabile la trascrizione **esatta** del parlato italiano. Non creare un ASR da zero: migliorare e misurare i motori esistenti; evitare lo STT di LUMO. L'utente preferisce rinviare le prove manuali fino a un collaudo cumulativo.

- Recuperato il prototipo `albatros-multi-stt-v2/` già presente nel ramo `albatros-ai-v1-3`; non è un sistema vocale pronto all'uso.
- Modificato `albatros-multi-stt-v2/engine_runner.py`: Whisper italiano beam size 5 modificabile (1–10), suggerimenti lessicali opzionali `WHISPER_ITALIAN_HINTS` (max 300 caratteri), mantenuto VAD e divieto di utilizzare il contesto precedente.
- Creato `albatros-multi-stt-v2/italian_quality.py` per valutazione offline WER/CER, frasi esatte, errori in verbi opposti, negazioni e numeri.
- Creati `tests/test_italian_quality.py` e `ITALIAN_STT_QUALITY.md`.
- **Non affermare che l'accuratezza italiana sia già aumentata:** mancano registrazioni di riferimento e misure reali. Non è stato installato alcun modello/servizio su DietPi; nessuna modifica a Home Assistant, Alexa, Orange Pi o LUMO.
- **Priorità successiva:** dati italiani volontari e privati, analisi di tagli audio/VAD, benchmark per motore, selettore prudente; solo poi collaudo Wyoming standalone e integrazione domestica.


## Speech+ Supervisor — ulteriore sviluppo (25 novembre 2026)

**Richiesta utente:** un vero agente AI che controlli l'intero processo di riconoscimento vocale italiano, non una semplice somma di trascrittori. Priorità assoluta alla fedeltà audio→testo. Mantenere i test domestici per il collaudo finale.

- Aggiunto `albatros-multi-stt-v2/supervisor.py`: prima fase **deterministica**, con verifica formato PCM16/16kHz mono, durata, livello, clipping; decide con accordo di motori indipendenti, blocca conflitti critici e non riscrive il testo pronunciato.
- Integrato supervisore nel flusso `engine_runner.run()`. La verifica avviene **dopo** aver raccolto le trascrizioni dei motori abilitati; non accetta subito due risultati concordi se un terzo contraddice l'azione. Un esito incerto restituisce stringa vuota al protocollo Wyoming.
- Creato `tests/test_supervisor.py`, con prove audio PCM sintetiche, accordi, conflitti e router mock. **Non dichiarare superati questi nuovi test senza esecuzione verificata.**
- Nessun audio reale esaminato; nessun modello STT installato o collaudato end-to-end. La futura parte agentica dovrà osservare qualità, scegliere escalation dei motori e apprendere da errori su dati autorizzati, sempre senza correggere inventando parole e senza agire su dispositivi.
- Rischio noto: il supervisore prudente può rifiutare frasi corrette con un solo motore o con disaccordi; va misurato il tasso di astensione insieme a WER/CER, errori critici e latenza.


## ALBATROS Speech+ Fusion — fusione trascrizioni (25 novembre 2026)

**Obiettivo utente:** un unico STT che raccolga il meglio dei trascrittori e restituisca con la massima fedeltà possibile le frasi pronunciate in italiano. Non promettere il 100% incondizionato; su input incerto meglio segnalare o ritentare che inventare.
- Creato `albatros-multi-stt-v2/fusion.py`: fusione conservativa parola per parola solo dove concordano almeno due famiglie di motori diverse (Whisper locale/Groq Whisper considerati correlati); rifiuta conflitti critici e disallineamenti di lunghezza.
- Collegata fusione a `supervisor.py` come opzione di recupero quando il consenso sull'intera frase fallisce.
- Creato `tests/test_fusion.py` con verifiche su fusioni sintetiche, motori correlati, parole discordanti e opposizioni.
- Documentati limiti e prossime migliorie in `ITALIAN_STT_QUALITY.md`: manca allineamento temporale, riascolto selettivo e misurazione con audio italiano autentico. Fusioni senza verifica acustica possono essere sbagliate.
- **Non è stato effettuato collaudo reale di accuratezza; nessuna modifica alla pipeline Alexa/Home Assistant/LUMO.** Test manuali differiti per decisione utente. Verificare esito GitHub Actions e non confondere con qualità vocale.


## Diagnostica microfono — Speech+ (25 novembre 2026)

**Nuovo requisito utente:** l'agente supervisore deve **indicare impostazioni appropriate per il microfono** in base all'audio ricevuto, non soltanto confrontare trascrittori. Vuole indicazioni su guadagno, VAD/pre-roll, volume, tagli iniziali/finali, rumore ed eco. Le impostazioni esatte richiedono misure e conoscere il dispositivo: non inventare parametri hardware.

- Aggiunto `albatros-multi-stt-v2/mic_diagnostics.py`: diagnosi offline PCM16 16kHz mono di livello dBFS, picco, RMS, clipping, durata, possibile attività ai bordi e raccomandazioni testuali. Non modifica automaticamente parametri o dispositivi.
- Aggiunto `tests/test_mic_diagnostics.py` con segnali sintetici.
- Integrata diagnosi in `engine_runner.py`: log dei problemi/consigli, senza trascrizioni o audio salvati. Prove manuali rimandate come da volontà utente.
- **Limitazione:** il livello ai bordi è solo indizio di parole tagliate; non identifica con certezza VAD errato. Eco, rumore e distanza non possono essere diagnosticati con certezza dal solo RMS/peak. Per configurazioni reali serviranno campioni audio e informazioni su microfono/satellite.
- Prima di affermare che il sistema funziona, verificare test automatici e qualità reale con campioni autorizzati. Nessuna modifica al firmware Alexa/LUMO o alla pipeline Home Assistant.


## Autocalibrazione microfono autonoma — 25 novembre 2026

**Nuovo requisito esplicito:** ALBATROS Speech+ deve regolare direttamente e autonomamente le impostazioni del microfono, non limitarsi a dare consigli. L'utente vuole rimandare i test manuali fino a un collaudo completo.

- Creato `albatros-multi-stt-v2/autocalibrate.py`: controllore offline con impostazioni simulate (gain_db, pre_roll_ms, end_silence_ms), modifiche entro limiti fissati, almeno tre registrazioni con segnali ripetuti, una sola modifica per volta, valutazione A/B con errori di trascrizione e falsi comandi critici, accettazione solo se migliora, altrimenti rollback.
- Creato `tests/test_autocalibrate.py` con test unitari per regolazione guadagno, VAD, rollback, limiti e regresso dei comandi critici.
- **Non ancora collegato ai veri microfoni.** Per applicare le regolazioni automatiche servono adattatori specifici per firmware/device (Alexa ESPHome, Orange Pi ecc.), lettura/scrittura dei parametri realmente supportati, snapshot e ripristino verificabile. Un'opzione astratta di gain/pre-roll non prova che l'hardware la esponga. Niente modifiche a Home Assistant, Alexa o LUMO.
- **Nessuna ottimizzazione con parlato reale ancora misurata**; le soglie in codice sono ipotesi iniziali, da verificare prima di permettere attuazione automatica. Le operazioni di calibrazione dovranno restare isolate e con consenso iniziale esplicito per ogni dispositivo. Non promettere 100% di trascrizione corretta.


## Speech+ calibrazione del segnale audio (25 novembre 2026)

**Requisito:** ALBATROS deve analizzare e calibrare direttamente l'**audio registrato**, non soltanto consigliare o modificare i parametri del microfono.
- Aggiunto `albatros-multi-stt-v2/audio_calibration.py`: modulo DSP offline per PCM16 mono/16kHz; misura RMS, picco, offset DC, saturazione; rimuove componente continua lenta e applica regolazione di livello limitata con margine anticlipping. Restituisce **sia audio originale sia copia elaborata** per confronto A/B. Non tenta di ricostruire parole già tagliate.
- Aggiunti test sintetici `tests/test_audio_calibration.py` (originale conservato, silenzio e clipping non alterati, offset e limiti).
- Workflow `.github/workflows/albatros-multi-stt.yml` aggiornato per eseguire test STT su ramo attivo `albatros-ai-v1-3`.
- **Modulo audio NON ancora inserito automaticamente nella pipeline STT:** per evitare che filtri non collaudati alterino il riconoscimento in produzione, prossimo passo collegamento con opt-in su banco di prova e confronto WER/CER originale vs processato.
- Nessuna modifica a firmware/dispositivi reali o Home Assistant. Collaudo audio reale rinviato come da richiesta; test automatici software da verificare.
