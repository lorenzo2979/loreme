# ALBATROS Speech+ — accuratezza dell'italiano (fase di sviluppo)

## Obiettivo
Trascrivere **fedelmente** l'audio, non correggere secondo l'intento presunto. Per comandi ambigui o conflittuali non eseguire azioni. Non dichiarare accuratezza perfetta prima di test ripetibili.

## Migliorie codificate
1. `engine_runner.py` imposta Whisper in italiano (`language=it`) e usa beam size 5 (personalizzabile 1–10 tramite `WHISPER_BEAM_SIZE`). `WHISPER_ITALIAN_HINTS` consente di suggerire vocaboli/nomi propri fino a 300 caratteri. I suggerimenti non sono trascrizioni garantite e possono introdurre bias. Non inserire nomi sensibili o frasi comando in `.env` pubblico.
2. `italian_quality.py` calcola WER e CER (distanza di edit), percentuale di frasi perfette e presenza di divergenze critiche: opposizioni, negazioni, numeri. Funziona completamente offline; nessuna registrazione viene inviata o salvata dallo script.
3. `tests/test_italian_quality.py` testa punteggi, comandi opposti, negazioni, cifre, normalizzazione e opzioni Whisper.

## Formato di valutazione (dataset privato)
```json
[
  {
    "category": "comando",
    "reference": "Non accendere la luce in cucina",
    "hypotheses": {
      "whisper_local": "non accendere la luce in cucina",
      "speech_to_phrase": "accendi la luce in cucina"
    }
  }
]
```
Sostituire gli esempi con trascrizioni ottenute da **registrazioni reali con consenso** e un riferimento verificato manualmente. Non caricare audio o conversazioni private nel repository pubblico.

Dalla cartella `albatros-multi-stt-v2`:
```bash
python -m unittest discover -s tests -v
python italian_quality.py dataset_privato.json
```
I risultati WER sono significativi soltanto su campioni rappresentativi, con rumore, accenti, distanza dal microfono, nomi di musica, frasi libere, conversazioni brevi e comandi con negazioni. Misurare separatamente falsi comandi eseguiti, rifiuti, latenza, accuratezza dei numeri e dei nomi. Mai ottimizzare sui soli campioni usati per progettare il sistema.

## Stato e limiti
Il router esistente Multi-STT V2 è **prototipale**: manca build/integrazione reale e confronto con audio italiano. Whisper, Speech-to-Phrase e provider cloud non sono stati ancora collaudati end-to-end nel medesimo sistema. Questa fase migliora strumenti e configurazione, **non prova** ancora un aumento misurato dell'accuratezza. Non cambiare pipeline HA e LUMO operativi.

## Prossima iterazione
- Dataset italiano personale consensuale, separato in sviluppo/validazione.
- Misurazione per motore e per tipo di frase: WER, CER, frasi esatte, errori critici, latenza.
- Confronto di modelli locali adatti all'hardware DietPi e criteri di selezione dell'output basati su prove; non affidarsi alla concordanza fra modelli come garanzia assoluta.
- Diagnostica di endpoint audio: VAD/taglio inizio/fine, volume, eco e rumore; distinguere riconoscimento acustico da trascrizione.
- Solo dopo: Wyoming standalone e integrazione controllata in Home Assistant.


## Fusione controllata delle parole — prototipo

- `fusion.py` combina **solo** parole effettivamente riconosciute da motori diversi, e soltanto se almeno due *famiglie di modelli* concordano sulla stessa parola nella stessa posizione. Whisper locale e Groq Whisper appartengono alla stessa famiglia e non valgono come due conferme indipendenti.
- Il supervisore `supervisor.py` usa la fusione come fallback quando non esiste una trascrizione completa concordante. Con negazioni/opposizioni discordanti, parole inserite o omesse, o parole senza conferma, restituisce richiesta di ripetizione (nel gateway attuale: testo vuoto).
- La versione attuale **non effettua allineamento temporale o riascolto del segmento audio**: può combinare erroneamente parole se le frasi hanno parole allineate per posizione ma riferite a segmenti diversi. L'accordo tra modelli non è garanzia di verità.
- I test `tests/test_fusion.py` usano **trascrizioni sintetiche**, non registrazioni reali. Non presentare questo lavoro come STT impeccabile; misurare i risultati su corpus italiano verificato prima di installarlo sulla pipeline domestica.

### Sviluppo successivo
Allineamento con timestamp e confidenze calibrate, selettore dei segmenti incerti, una seconda decodifica basata sull'audio originale, verifica negazioni e numeri; segnalare esplicitamente incertezza quando l'audio non permette una conclusione. Misurare WER, percentuale di frasi completamente corrette, tasso di astensione, falsi comandi ed effettiva latenza.
