# ALBATROS Multi-STT — ricerca comparativa e feedback utenti (24-11-2026)

## Ambito
Confronto tra discussioni Home Assistant Community, GitHub Issues ufficiali e Reddit. Opinioni degli utenti = evidenze aneddotiche, non benchmark comparabili. Non confondere accuratezza del riconoscimento (STT) con la capacità di Home Assistant / Music Assistant di interpretare la frase.

## Risultati rilevanti
1. **Speech-to-Phrase ha dei falsi riconoscimenti**: in OHF-Voice/speech-to-phrase issue #138 (20 luglio / 8 settembre 2026) un utente riporta N-best corretto al primo posto, ma decoder che sceglie la seconda frase con una stanza extra. Implicazione: non considerare automaticamente valido un output STP, e attenzione a nomi di stanze/dispositivi.
   https://github.com/OHF-Voice/speech-to-phrase/issues/138
2. **Fall-back reale e utile**: thread Home Assistant del 19 marzo 2026, un utente usa STP→Faster-Whisper con una implementazione derivata da PR #17 di Wyoming Addons e riferisce che funziona. Implicazione: integrare un fallback in un unico endpoint ha precedenti funzionanti, ma ALBATROS rimane diverso perché non deve chiamare le entità STT di Home Assistant.
   https://community.home-assistant.io/t/speech-to-phrase-fallback-to-faster-whisper-no-cloud/997205
3. **Trade-off velocità/accuratezza**: thread Home Assistant 2025-2026 riportano Speech-to-Phrase rapido per frasi note, modelli Whisper piccoli che peggiorano nomi/lingua, modelli medi più accurati ma più lenti su CPU. Non assumere RAM/latency di DietPi senza misure.
   https://community.home-assistant.io/t/assist-fallbacks/865137
   https://community.home-assistant.io/t/help-in-finding-a-reliable-stt-for-italian/869203
4. **Problema nomi di artisti in Music Assistant**: conversazione Reddit r/homeassistant novembre 2026 riferisce errori su nomi propri con Whisper e fuzzy matching nel player. Implicazione: includere nel corpus di test i nomi reali di cantanti, per esempio «Vasco Rossi», «Eros Ramazzotti», «Laura Pausini», senza una libreria musicale locale.
   https://www.reddit.com/r/homeassistant/comments/1xvqcy9/voice_incubator/
5. **Nuovo Speech-to-Phrase**: issue ufficiale #149, 30 settembre 2026: codice Kaldi legacy fino 1.4.3; nuova versione in OHF-Voice/apps con decoder CTC vincolato e supporto italiano; i modelli/grammatiche non sono intercambiabili. Issue #130, 28 settembre 2026: sviluppatore segnala che per beta è disponibile esclusione delle frasi builtin ma il container Docker standalone era ancora da preparare in quel momento. Verificare commit recenti prima di scegliere implementazione.
   https://github.com/OHF-Voice/speech-to-phrase/issues/149
   https://github.com/OHF-Voice/speech-to-phrase/issues/130
6. **Wyoming fallback esistente**: rhasspy/wyoming-addons usa entità STT e token Home Assistant, perciò NON soddisfa il requisito ALBATROS di indipendenza da HA (è utile soltanto come riferimento per buffering/fallback).
   https://github.com/rhasspy/wyoming-addons/blob/master/stt-fallback/DOCS.md
7. **Faster-Whisper CPU**: guida ufficiale indica modello CPU int8 con CTranslate2. Modello da scegliere misurando resident memory e tempo reale sul processore DietPi, non in base a benchmark GPU/altre macchine.
   https://github.com/SYSTRAN/faster-whisper
8. **Cloud gratuito**: una chiave Groq può abilitarne l'STT su account con limiti; la chiave Google AI Studio non equivale a Google Cloud Speech-to-Text. Non attivare cloud senza autorizzazione e rate limiting.

## Modifiche da considerare all'agente
- Modalità `fast`: STP prima, verificare trascrizione tramite regole grammaticali locali ad alta specificità, poi eventualmente Whisper. Evitare invii di comandi potenzialmente pericolosi con un solo modello.
- Modalità `judge` (predefinita per ALBATROS V2 sperimentale): richiedere consenso di almeno due STT, fermandosi quando sufficiente. È prudente ma può aumentare latenza.
- Compiti delicati: non trattare come equivalenti accendi/spegni, apri/chiudi, aumenta/diminuisci, numeri o nomi diversi di stanze.
- Preferire l'astensione a un'azione sbagliata, con richiesta di ripetizione; registrare solo nome del motore, esito e latenza, **non audio o trascrizioni sensibili**.
- Collaudare con corpus italiano locale autorizzato: 20 comandi domotici, 20 artisti, 10 frasi fuori grammatica, 10 condizioni di rumore; raccogliere tempo medio, percentuale di risposta corretta e astensioni.
- Non assumere che due motori concordanti siano corretti: possono condividere errori.
- L'agente restituisce una sola trascrizione via Wyoming a Home Assistant, senza chiamare le API degli STT di Home Assistant.

## Blocchi non risolti
- Unico Docker contenente Speech-to-Phrase locale reale e Faster-Whisper locale, build e process-manager ancora da collaudare.
- Compatibilità API reale per Groq e Gemini, timeout, quote e modello abilitato.
- Test sul dispositivo DietPi dell'utente (nessun accesso remoto) e confronto con audio italiano reale.
- GitHub Actions senza esecuzione confermata; non dichiarare CI verde.

## Decisione
NON cambiare la pipeline Alexa esistente; non modificare Music Assistant né il firmware. Continuare il ramo `albatros-multi-stt-v2-experimental` finché build e test end-to-end saranno verificati.


## Altri progetti e riscontri individuati
9. **OHF-Voice Voice Incubator**, novembre 2026: modello di riconoscimento vocale, correzione dei nomi e sessioni Wyoming, anche con ESPHome. La modalità standalone richiede tuttavia HASS_URL e HASS_TOKEN, quindi **non è accettabile come base diretta** per ALBATROS, che non deve importare dati da HA. Da studiare soltanto i principi di segmentazione/valutazione.
   https://github.com/OHF-Voice/voice-incubator
10. **hass-closest-intent**, maggio 2026: esempi di normalizzazione fuzzy per comandi storti. Segnalate anche possibili correzioni eccessive. In ALBATROS evitare auto-correzioni di parole che possono invertire un comando o cambiare destinatario.
   https://community.home-assistant.io/t/hass-closest-intent-garbled-stt-output-in-actual-intent-out/1009765
11. **Reddit**: utilizzatori di Home Assistant riferiscono buona velocità di Speech-to-Phrase, ma scarsa flessibilità per richieste musicali e nomi propri; modelli Whisper grandi possono consumare molte risorse. Sono testimonianze, non misure riproducibili.
   https://www.reddit.com/r/homeassistant/comments/1ioskrl/speechtophrase_brings_voice_home_voice_chapter_9/
   https://www.reddit.com/r/homeassistant/comments/1xvqcy9/voice_incubator/

## Nuovo strumento del repository
`evaluate.py`: valuta trascrizioni di test autorizzate senza inviare audio nel cloud. Produce percentuale di frasi corrette, astensioni e comandi errati. Esecuzione: `python evaluate.py percorso/dataset.json`. Evitare l'inserimento di parole private nel repository pubblico; usare dati sintetici o conservare i test reali fuori da GitHub.


## Nuova ricerca mirata: progetti con codice e dati di prova

12. **incendiary/Chorus**: implementazione Docker di più passaggi STT e votazione Levenshtein. L'autore pubblica un benchmark (luglio 2026, 15 frasi inglesi) con WER 0,0314 → 0,0288 su audio pulito ma 0,1024 → 0,1095 su audio rumoroso: il consenso NON migliora necessariamente l'accuratezza. Importante: ALBATROS deve mantenere possibilità di astensione e misurare prestazioni su campioni italiani, senza assumere che due sistemi concordanti abbiano ragione.
   https://github.com/incendiary/Chorus
13. **roryeckel/wyoming_openai**: un server Wyoming con backend API OpenAI-compatibili, include esempio Docker Groq e test end-to-end su Home Assistant di prova; utile per sostituire il client Groq scritto a mano e per progettare collaudi riproducibili. Non integra Speech-to-Phrase e Whisper nello stesso container come richiesto.
   https://github.com/roryeckel/wyoming_openai
14. **mitrokun/asr_proxy extended**: in una discussione Reddit l'autore racconta di fare un matching progressivo contro dizionario di comandi e risposta anticipata; promettente per la velocità, richiede test in italiano e compatibilità con STP.
   https://github.com/mitrokun/asr_proxy
   https://www.reddit.com/r/homeassistant/comments/1r3j01l/i_built_a_speaker_verification_proxy_that_filters/
15. **ndom91/ha-voice-rocm**: sperimenta più motori Wyoming in Docker, ma in container separati. Risolve l'uso di vari STT, NON la confezione All-in-One ALBATROS.
   https://github.com/ndom91/ha-voice-rocm
16. **ZFTurbo/asr_ensemble**: funzioni pronte per il voto weighted/median su N trascrizioni; valutare libreria/licenza prima del riuso, senza assumere sempre migliore dei singoli STT.
   https://github.com/ZFTurbo/asr_ensemble

**Raccomandazione tecnica derivata:** due modalità nello stesso router: `fast` per un comando locale chiaramente identificato con politiche rigorose e `verify` per richiesta dubbia, privilegiando integrità del verbo e nome dell'entità. Avviare il fallback soltanto quando ha senso; evitare di inviare al cloud tutta la domotica routinaria. Verificare con corpus italiano prima di abilitare `fast` in produzione.


## Terza ricerca mirata — server STT multipli esistenti

17. **hass-cortex/cortex-stt**: server multiplo (Whisper, Parakeet, SenseVoice, Qwen3-ASR, ecc.), streaming e gestione dei modelli. L'integrazione HA crea però una entità STT per ogni modello, non un agente unico che vota e inoltra un solo risultato. Studiare struttura dei motori locali.
   https://github.com/hass-cortex/cortex-stt
18. **ManiAm/Speak-IO**: API modulare con motori Whisper, Faster-Whisper, whisper.cpp, Vosk e deploy Docker Compose con più servizi. Ottimo riferimento per interfacce dei provider; non rispetta la richiesta di un container unico senza adattamenti.
   https://github.com/ManiAm/Speak-IO
19. **bogdanr/fono**: provider STT cloud Groq e locali, funzionalità Wyoming opzionali. Potenziale riferimento per credenziali e selezione dei provider; non verificata integrazione Speech-to-Phrase locale nello stesso contenitore.
   https://github.com/bogdanr/fono/blob/main/docs/providers.md
20. **snabb/wyoming_openrouter**: matrice di test reale STT/TTS con misurazione di costi e latenza, riproducibile. Utile per il piano di collaudo provider, non sostituisce il router.
   https://github.com/snabb/wyoming_openrouter
21. **steemandavid/whisper-transcribe**: ROVER e correzione LLM nel container per trascrizioni, ma ottimizzato per audio lungo e GPU; non adatto senza cambiamenti al controllo domotico in tempo reale.
   https://github.com/steemandavid/whisper-transcribe

**Esito:** non è emerso un progetto esistente verificato che includa contemporaneamente Speech-to-Phrase interno, Whisper locale, Groq, Gemini, valutazione di tutte le trascrizioni, Wyoming e un unico container Docker senza dipendenze STT di Home Assistant. Trattare i progetti sopra come reference design; verificare licenze prima di copiare codice.


## Quarta ricerca: fonti ESTERNE ai siti già consultati
Fonti primarie: Go Packages, documentazione tecnica OpenVoiceOS, siti indipendenti e mirror pubblici del codice.

22. **kombify SpeechKit: router STT provider modulari in Go**. La pagina pubblicata su pkg.go.dev documenta la funzione BuildRouter che costruisce in ordine stabile un router multi-provider con Groq, Google, Gemini, whisper.cpp (locale) e provider cloud. IsGeminiTranscribeModel distingue i modelli che usano Gemini API da Google Cloud STT. È il riferimento strutturale più vicino ad ALBATROS, ma NON incorpora Speech-to-Phrase né verifica concordanza fra trascrizioni di diversi motori.
    https://pkg.go.dev/github.com/kombifyio/SpeechKit/pkg/speechkit/x/stt/allproviders
    https://github.com/kombifyio/SpeechKit
23. **SpeechRouter** (speechrouter.ai): gateway open source Apache-2.0, self-host con Docker, 17 provider, fallback persino a metà frase e buffering audio. Supporta Groq, Google e Gemini. Il fallback ai guasti non coincide con un giudice di accuratezza STT e un gateway potrebbe mantenere molti servizi esterni. Studiare protocollo normalized e replay; non sostituire requisito container unico.
    https://speechrouter.ai/
    https://github.com/speech-router/speechrouter
24. **OpenVoiceOS STT HTTP Server**: guida tecnica indipendente con modello plugin STT, fallback del listener e API compatibili con diversi vendor (Groq, Google, etc.). Non va usato il fallback a server STT pubblici senza consenso: ALBATROS deve rispettare preferenza per STT locale e privacy. Docker example esiste, ma di norma contiene UN motore per server.
    https://openvoiceos.github.io/beta-technical-manual/stt-server/
    https://openvoiceos.github.io/beta-technical-manual/stt-plugins-reference/
25. **Hermes Agent, storico commit pubblico su mirror alternativo**: implementata catena local faster-whisper > Groq > OpenAI con funzioni separate e modello locale singleton. Un commit successivo disabilita i retry API automatici a causa di pause eccessive con HTTP 429; altro commit aggiunge timeout espliciti. Indicazione concreta per ALBATROS: su 429 passare subito a Google solo se abilitato e nel suo limite, e non bloccare Assist per minuti. Questo progetto non contiene Speech-to-Phrase principale o Gemini nel medesimo container.
    https://git.cosmolan.fr/mirrors/hermes-agent/commits/commit/54318c65b06fba764d7f8375f82d2818c01a3e67/tools/transcription_tools.py
26. **OpenVoiceOS container STT** supporta un engine primario e fallback opzionale Vosk via plugin; prova che la catena è implementabile in un ambiente self-hosted ma non comprende il giudice multi-STT.
    https://openvoiceos.github.io/ovos-docker/getting-started/docker/plugins/stt/
27. **Confronto misurato indipendente**: esistono benchmark multicategoria open-source (ad es. verli.app), ma non trarre conclusioni sulle prestazioni in italiano di DietPi usando prove in inglese o su GPU. Usare parole e frasi italiane con audio registrato sotto consenso.
    https://verli.app/blog/speech-to-text-accuracy-benchmark

### Decisioni implementative
- Il router deve distinguere errore tecnico/timeout da trascrizione dubbia; i due casi richiedono politiche differenti.
- Rate limit cloud: nessun retry lungo automatico su HTTP 429; passare a fallback quando configurato, senza spesa non prevista.
- Cloud disabilitato per default; l'audio non deve essere inviato fuori dalla LAN prima di opt-in.
- Evitare la correzione semantica fantasiosa dei nomi degli artisti; giudice a consenso e possibilità di astenersi.
- Speech-to-Phrase deve partire come motore locale dentro il **singolo** container, con frasi locali gestite separatamente da Home Assistant.
- Conclusione: anche questi esempi NON costituiscono una soluzione plug-and-play verificata che soddisfi tutti i vincoli ALBATROS.


## Approfondimento tecnico dei quattro candidati (24 novembre 2026)

### 1. Kombify SpeechKit
- Provider model `STTProvider`, `Router.Route`, `BuildRouter`, `Register` per driver custom; `IsGeminiTranscribeModel` distingue Gemini API da Google Cloud Speech API.
- Locale: whisper.cpp gestito attraverso un processo server figlio dal componente local. Stile: libreria + host Go. License Apache-2.0 dai metadati Go Packages.
- **Riutilizzabile**: contratti uniformi, ordering, lifecycle/health del provider, differenziazione Google API.
- **Incompatibilità**: Go vs attuale Python, nessun Speech-to-Phrase già pronto; routing su errori non equivale a judge di accuratezza.
Fonti:
https://pkg.go.dev/github.com/kombifyio/SpeechKit/pkg/speechkit/stt
https://pkg.go.dev/github.com/kombifyio/SpeechKit/pkg/speechkit/x/stt/allproviders
https://pkg.go.dev/github.com/kombifyio/SpeechKit/pkg/speechkit/x/stt/local
https://pkg.go.dev/github.com/kombifyio/SpeechKit/pkg

### 2. SpeechRouter
- Gateway stream WebSocket / batch HTTP, 17 provider e 44 modelli dichiarati, PCM16 16kHz mono, failover *midstream* con replay dei frame bufferizzati e soppressione dei final duplicati.
- Deploy self-host mediante Docker Compose, credenziali BYOK, license Apache-2.0 dichiarata.
- **Riutilizzabile**: audio buffer/replay, normalizzazione e error codes, timeout, conferma di un solo messaggio finale.
- **Incompatibilità**: focalizzato su providers cloud, non integra di per sé Speech-to-Phrase e Whisper locali né consensus accuracy. BYOK gratuito per gateway non rende necessariamente gratuito il provider sottostante.
Fonti:
https://github.com/speech-router/speechrouter
https://docs.speechrouter.ai/
https://docs.speechrouter.ai/guides/audio-persistence/

### 3. OpenVoiceOS STT
- `ovos-stt-http-server` è server FastAPI che avvia un plugin `--engine` configurabile; ha Dockerfile standard.
- `ovos-dinkum-listener` permette `fallback_module` quando primario non restituisce testo, NON quando restituisce testo plausibile ma errato.
- **Riutilizzabile**: interfacce Python e caricamento provider, plugin e config.
- **Incompatibilità**: tipicamente un engine per server, fallback a due livelli, non valutazione di quattro trascrizioni, non Wyoming nativo del server documentato.
Fonti:
https://openvoiceos.github.io/beta-technical-manual/stt-server/
https://openvoiceos.github.io/ovos-docker/getting-started/docker/plugins/stt/

### 4. Hermes Agent
- Documentazione Voice Mode indica fallback `local > groq > openai`, Faster-Whisper e language hint; configurazione STT per provider.
- ATTENZIONE: issue #105901 (8 settembre 2026) segnala che il percorso condiviso di trascrizione non gestisce il fallback generic-provider in ogni situazione. Distinguere percorsi caller e revisioni: non copiare assumendo una feature universale.
- **Riutilizzabile**: timeout/rate limits/gestione provider e STT local + Groq.
- **Incompatibilità**: è un agent completo per messaging e non un server Wyoming con Speech-to-Phrase/Gemini integrati; necessità di verificare ogni modalità di fallback.
Fonti:
https://hermes-agent.nousresearch.com/docs/user-guide/features/voice-mode
https://hermes-agent.nousresearch.com/docs/user-guide/configuration
https://github.com/NousResearch/hermes-agent/issues/105901

### Matrice di decisione ALBATROS
- Router unico Docker Python: mantenere implementazione proprietaria per ora, ispirandosi ai contratti di SpeechKit, senza introdurre un runtime Go.
- STP locale: necessaria prova di avvio e grammatica autonoma con italiano.
- Whisper locale: preferire Python `faster-whisper` a processi multipli inutili; preload in RAM con limiti.
- Groq: client dedicato o API standard con timeout e zero retry automatici su 429.
- Gemini: API Google AI Studio e modello audio realmente compatibile con free tier, **non** Cloud Speech-to-Text.
- Wyoming: conservare una sola registrazione originale e restituire un solo transcript all'Assist.
- Judge: validazione semantica conservativa per opposizioni, numeri e nomi; non basta consenso testuale. Per azioni rischiose preferire astensione.
- Non trasferire dati HA all'agente; il trasporto audio via Assist rimane l'unico collegamento di ingresso.
- **Non promuovere allo stato pronto** fino a build Docker reale, test end-to-end e misurazioni su DietPi.

Nessun progetto soddisfa attualmente tutti i vincoli del nostro All-in-One senza ulteriore integrazione.


## Quinta ricerca: motori a grammatica indipendenti da HA (fonti nuove, 24/11/2026)

28. **Rhasspy Speech (Kaldi standalone)**: usa template di frasi e modelli precompilati anche per l'italiano; il codice crea due modelli di riconoscimento (grammatica rigida e language model meno vincolato) e confronta le uscite per rifiutare parole fuori vocabolario. È una base realmente indipendente dalla lettura delle entità HA se configurata con file locali; esiste immagine `rhasspy/wyoming-rhasspy-speech` su Docker Hub. **Non è la nuova versione di Speech-to-Phrase**, quindi è candidato alternativo/fallback di ricerca, non sostituto approvato. Gli strumenti Kaldi/OpenFST/OpenGRM/Phonetisaurus e compatibilità della build nello stesso container restano da verificare.
   https://github.com/rhasspy/rhasspy-speech
   https://hub.docker.com/r/rhasspy/wyoming-rhasspy-speech
29. **Wyoming Vosk, corrected/limited**: ha già server Wyoming e template YAML locali in tre modalità (open, corretto, limitato), con RapidFuzz. I modelli piccoli sono veloci, ma la correzione forzata può inventare comandi falsi se il cutoff è troppo permissivo. Non ha bisogno di leggere entity HA per riconoscere frasi preimpostate; NON incorpora la capacità STT open-ended di Whisper.
   https://github.com/rhasspy/wyoming-vosk
30. **Caso utente italiano**: testimonianza di utente che ha installato Vosk con italiano, riscontrato inserimenti spurii (es. «e» prima di ogni frase), quindi modificato le frasi/template ottenendo risultati più affidabili. Pubblica modifiche `simmessa/wyoming-vosk-standalone-IT_language`. È utile come corpus di regressione in italiano, ma non dimostra prestazioni sul nostro DietPi né dei quattro STT.
   https://www.reddit.com/r/homeassistant/comments/1hu28a4/
   https://github.com/simmessa/wyoming-vosk-standalone-IT_language
31. **Ricerca comparativa Home Assistant italiana**: thread «faster-whisper slow and Ita model weak» racconta che Vosk è più veloce di Whisper, con difetti sulle prime parole; gli utenti compensano con regole locali. Utilizzabile per definire test con prefissi «e», «eh», comandi contrari e nomi di cantanti.
   https://community.home-assistant.io/t/voice-assistant-faster-whisper-slow-and-ita-model-weak-any-support-about/689470
32. **STP legacy (Kaldi) e riscrittura moderna**: la modalità Docker Wyoming documentata di Speech-to-Phrase legacy richiede HA websocket/token per generazione di grammatica. Non trattare l'opzione `--custom-sentences-dir` come prova di avvio senza HA: verificare separatamente se training standalone è disponibile. La riscrittura CTC ha stack diverso e richiede un'integrazione nuova.
   https://github.com/OHF-Voice/speech-to-phrase
   https://github.com/OHF-Voice/apps

**Decisione aggiornata**: provare a conservare Speech-to-Phrase moderno come motore #1 senza leggere dati HA. Se impossibile, proporre espressamente all'utente Rhasspy Speech come alternativa a grammatica locale (senza sostituirlo di nascosto). Mantenere un solo container finale e un solo endpoint Wyoming. Prima della build: proof of concept standalone, poi modelli italiani e CPU/RAM.
