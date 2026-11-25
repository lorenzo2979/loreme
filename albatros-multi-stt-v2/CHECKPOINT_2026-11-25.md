# ALBATROS — CHECKPOINT 25 novembre 2026
## Stato congelato: Supervisore Intelligente V1.1
Ambito: agente audio, calibrazione digitale e Multi-STT italiano. Non è un checkpoint dei progetti CAD o di altri dispositivi. Repository `lorenzo2979/loreme`, ramo `albatros-ai-v1-3`, directory `albatros-multi-stt-v2/`. Commit del codice da cui ripartire: `e3d9b0474f1ee64e6060565d67349660418b9348`. Questo documento non modifica firmware, configurazioni o codice eseguibile.

### Obiettivo e vincoli
- Trascrivere fedelmente l'italiano anche in presenza di rumore, senza inventare o completare parole mancanti.
- Analizzare e calibrare automaticamente il segnale, selezionare filtri e fusioni, valutare l'incertezza; successivamente regolare il microfono fisico con verifiche reali.
- Conservare PCM originale; PCM16 mono 16 kHz, durata 0,25–30 s. Niente tagli arbitrari o ricostruzione di parole. Proteggere negazioni, numeri e comandi opposti.
- Non modificare il firmware Alexa ESPHome funzionante né Home Assistant; nessuna nuova funzione attivata di default. Non presentare l'accordo di un solo modello su più audio come indipendenza di più STT.
- Il sistema Multi-STT esistente confronta Speech-to-Phrase, Whisper locale, Groq, Gemini; accettazione prudente con famiglie indipendenti. La qualità del sistema completo non è ancora verificata.

### Cronologia tecnica e riferimenti
| Versione | Commit | Run GitHub | Esito |
|---|---|---|---|
| Audio agent V1 | ec93db9f3f64afdd970244c22bd71afeb967c3f0 | 45674964726 | test superati |
| Benchmark V1 | fd64fdf998412d980910b2a8c2bc68d2a0de118b | 45688330540 | 50 prove; 50 → 48 errori |
| Filter bank iniziale | a4113188bfe9fad560c89342607d818b2f8ab100 | 45703941215 | 5 moduli falliti |
| Filter bank corretto | 6de9bb1cc3995688074238a4ddb5f7599eba98dd | 45705173493 | 8/9 riusciti; WebRTC non installato |
| Fusion audio | 9ebf48277ce3e533855dfee76a0321531a53f13a | 45708799293 | 9/9 riusciti |
| Supervisore V1.0 | 9099639a9d2ad93d72b1c4d3541d521cfe58d2b7 | 45711469007 | unit test ok; benchmark indipendente interrotto |
| Supervisore V1.1 | e3d9b0474f1ee64e6060565d67349660418b9348 | 45712632697 | 7 test mirati ok; 50 prove concluse; benchmark indipendente interrotto |
| Regressione generale V1.1 | e3d9b0474f1ee64e6060565d67349660418b9348 | 45712632623 | 71 test eseguiti, 2 errori import: scipy mancante |

I run sono consultabili su https://github.com/lorenzo2979/loreme/actions/runs/ seguito dall'ID.

### Stato dei filtri (run fusion audio 45708799293)
50 condizioni per modulo, 330 parole di riferimento per confronto: 10 clip italiane (2 parlanti), originali e rumore bianco 10 dB, ventilatore simulato 5 dB, ronzio 10 dB, volume -18 dB. Whisper Base CPU int8. Le baseline oscillano fra job (50/51 errori); comparare ogni variante con la baseline del suo job. Percentuali WER, più basse sono migliori.

| Filtro | Originale | Filtro 100% | Mix 25% | Mix 50% | Mix 75% |
|---|---:|---:|---:|---:|---:|
| ALBATROS V1 | 15,15 | 14,55 | 15,76 | 14,24 | 14,55 |
| DPDFNet | 15,45 | 13,94 | 15,15 | 14,55 | 14,85 |
| DeepFilterNet | 15,15 | 14,24 | 16,06 | 15,45 | 15,76 |
| WebRTC | 15,15 | 15,45 | 13,64 | 13,33 | 13,64 |
| RNNoise | 15,15 | 19,39 | 15,15 | 16,36 | 16,67 |
| GTCRN | 15,15 | 20,91 | 16,06 | 15,45 | 15,15 |
| FFmpeg | 15,45 | 15,76 | 15,76 | 15,15 | 15,15 |
| AGC | 15,15 | 14,55 | 15,76 | 14,55 | 14,85 |

Mix indica quota di audio filtrato, resto originale. WebRTC 50%: 44 vs 50 errori, 4 prove migliorate, 3 peggiorate, 43 invariate. WebRTC 25%: 45 errori, 2 migliorate, 0 peggiorate. DeepFilterNet dopo correzione del livello: 47 errori vs 414 nel run precedente; non significa qualità perfetta. Silero VAD rileva voce in tutte le 50 prove, senza validazione precisa dei confini. Tutti i 9 job hanno conclusione success. Non confondere esecuzione corretta con miglioramento.

### Supervisore: architettura e file
- `audio_filter_bank.py`: 8 filtri, più originale; Silero VAD diagnostico. Ogni filtro parte da originale. Modelli e dipendenze espliciti.
- `audio_fusion.py`: correlazione e allineamento entro ±50 ms, bordi originali, blend 25/50/75%.
- `audio_agent.py`, `audio_calibration.py`, `mic_diagnostics.py`, `autocalibrate.py`: diagnostica e calibrazione digitale, non hardware.
- `audio_supervisor.py`: profili quiet/tonal/broadband/uncertain; routing a budget limitato; protezioni su attenuazione vocale, correlazione e picco; confronto Whisper; FeedbackBook con riferimenti verificati, conteggi aggregati e soglia 10 osservazioni. V1.1 consente alternative singole con margini 0,15 (feedback favorevole) o 0,35 (rumore tonale/broadband) e piccoli edit non critici.
- `engine_runner.py`: integrazione sperimentale opzionale `ENABLE_AUDIO_SUPERVISOR=true` con `ENABLE_WHISPER=true`; lascia default invariati. `supervisor.py`, `agent.py`, `fusion.py`: giudizio Multi-STT.
- `audio_supervisor_benchmark.py`, `filter_bank_benchmark.py`, `audio_agent_benchmark.py`, `italian_quality.py`: misure WER/CER, errori critici, confronto per condizione.
- `tests/test_audio_supervisor.py`, `tests/test_audio_fusion.py`; workflow `.github/workflows/albatros-audio-supervisor.yml` e `.github/workflows/albatros-filter-bank.yml`; documentazione `AUDIO_SUPERVISOR.md`, `AUDIO_FUSION_EXPERIMENT.md`, `AUDIO_FILTER_BANK_V2.md`.

### Ultimo collaudo: V1.1, 25/11/2026 ore 23:22–23:25:58 CET
Run https://github.com/lorenzo2979/loreme/actions/runs/45712632697
Job 139550648060. Installazione, sette test mirati, preparazione modelli e upload artifact completati; benchmark complessivo fallito.
| Risultato sulle 50 prove di sviluppo | Originale | Supervisore | Oracle diagnostico |
|---|---:|---:|---:|
| Errori su 330 parole | 50 | 50 | 37 |
| WER | 15,15% | 15,15% | 11,21% |
| Frasi esatte | 44% | 44% | 48% |
| Casi con errori critici | 0 | 0 | 0 |

Selezioni originali: 50/50; 0 migliorate, 0 peggiorate, 50 invariate. Oracle usa il riferimento umano per scegliere retrospettivamente il candidato migliore; non è un risultato raggiunto dal supervisore né un metodo utilizzabile in produzione. Potenziale relativo 26% di errori in meno sul solo corpus di sviluppo. Il V1.0 aveva lo stesso WER e nessun cambio. Feedback di sviluppo mostra DPDFNet e WebRTC promettenti in profilo broadband, ma campione esiguo; non dimostra generalizzazione.

### Blocchi verificati (non risolti)
1. V1.0: download parquet FLEURS oltre 750 MB. V1.1: API datasets-server /rows per google/fleurs, it_it, test, HTTP 500. Nessuna delle 72 prove indipendenti eseguita. Corpus previsto: 24 registrazioni FLEURS distinte, pulite e miscelate con rumore registrato DEMAND DKITCHEN e SPSQUARE; miscele artificiali, non registrazioni reali del satellite.
2. Il selettore rimane troppo conservativo. I filtri offrono alternative migliori nel confronto retrospettivo, ma le regole attuali non le selezionano. Inoltre l'apprendimento viene disattivato nel percorso train per evitare leakage; la politica congelata non è ancora stata valutata su holdout. Non dichiarare agente addestrato con successo.
3. Regressione generale https://github.com/lorenzo2979/loreme/actions/runs/45712632623 job 139550647348: 71 test, due errori ImportError in test_audio_fusion e test_audio_supervisor: ModuleNotFoundError: No module named 'scipy'. Workflow dedicato installa scipy e passa. Non è stato ancora corretto il workflow generale.
4. Nessuna prova sul microfono fisico Alexa, nessun AEC con riferimento di playback, nessuna validazione di latenza su hardware reale, nessun apprendimento persistente automatico o controllo reale del gain. Nessun deploy in Home Assistant.

### Ripartenza prioritaria (proposta, non eseguita)
1. Sistemare dipendenze CI generale e rieseguire regressioni; mantenere dipendenze pin verificabili.
2. Preflight FLEURS con download diretto di archivio ufficiale o parquet con limite adeguato/streaming e fallback indipendente documentato. Verificare licenza, integrità, trascrizioni e rumori DEMAND; salvare manifest degli esempi, separare acquisizione da benchmark per evitare perdere il lavoro. Se fonte non disponibile, segnalarlo; non sostituire silenziosamente con dati di training.
3. Separare training, validazione (speaker disgiunti) e holdout; raccogliere molti più esempi, includere parole brevi, negazioni, numeri e comandi italiani, rumori reali e parlato lontano. Nessun riferimento del test nel selettore.
4. Registrare caratteristiche audio e affidabilità per parola (Whisper word timestamps/probabilities, attenzione a calibrazione non probabilistica), margini, errori di omissione/inserimento; addestrare ranker/regole con feedback verificato e validazione separata. Valutare anche selezione del filtro a livello audio prima dello STT, non generare parole.
5. Confrontare originale, migliore filtro fisso, supervisore, oracle solo diagnostico; WER/CER, frasi esatte, errori critici, migliorate/peggiorate, latenza p50/p95, copertura e incertezza. Gate di rilascio da definire con dati indipendenti: miglioramento misurabile senza aumento di errori critici, non solo successo del workflow.
6. Solo dopo verifiche, testare Multi-STT e satellite reale; controllo microfono con impostazioni documentate, limiti e rollback. Mantenere firmware Alexa funzionante e opzioni sperimentali disattivate.
7. Nessun nuovo collaudo avviato da questo checkpoint. Per riprendere, leggere questo file e verificare stato branch/CI prima di modificare codice.

### Risorse pubbliche previste
- FLEURS: https://huggingface.co/datasets/google/fleurs
- DEMAND: https://zenodo.org/records/1227121
- Corpus di sviluppo: https://github.com/Nexdata-AI/347-Hours-Italian-Speech-Data-Collected-by-Mobile-Phone
Verificare licenze e disponibilità prima di utilizzi ulteriori. Nessun audio o credenziale incluso nel checkpoint.
