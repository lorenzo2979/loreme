# ALBATROS Speech+ — prima prova su audio italiano autentico (25 novembre 2026)

## Origine e metodo
- Audio pubblico di esempio nel repository Nexdata-AI/347-Hours-Italian-Speech-Data-Collected-by-Mobile-Phone, file `T0263G0002S0001.wav`; testo corrispondente `T0263G0002S0001.txt`. Materiali scaricati **solo sul runner temporaneo**, non inseriti nel repository ALBATROS; verificare la licenza prima di ridistribuirli.
- Riferimento manuale: **Durante i primi secoli dell'era cristiana**.
- 16 kHz mono PCM16, durata 4.028 s. Test con faster-whisper `tiny` (CPU/int8, italiano, beam size 5), campione pulito e rumore gaussiano sintetico a SNR 10 dB (seed fisso).
- Quattro versioni per ciascun audio: originale, guadagno+filtro DC, denoiser spettrale `noise_reduction.py`, denoiser+calibrazione `audio_calibration.py`.
- Trascrizione confrontata con `italian_quality.py` (WER/CER). Script ripetibile: `real_audio_probe.py` e workflow `.github/workflows/albatros-real-audio.yml`.
- [Run originale+calibrazione 45672250665](https://github.com/lorenzo2979/loreme/actions/runs/45672250665) **success**.
- [Run con denoiser 45672533330](https://github.com/lorenzo2979/loreme/actions/runs/45672533330) **success**.

## Risultati della seconda esecuzione (WER)

| Input | Originale | Solo calibrazione | Solo riduzione rumore | Riduzione+calibrazione |
|---|---:|---:|---:|---:|
| Pulito | 0.3333 | 0.3333 | 0.3333 | 0.3333 |
| Rumore 10 dB | 0.1667 | 0.3333 | 0.1667 | **0.0000** |

Nel caso rumoroso con entrambe le elaborazioni il testo riconosciuto è stato **«durante i primi secoli dell'era cristiana»**, perfettamente coincidente con il riferimento dopo normalizzazione; non ci sono stati errori parola-per-parola in quel campione.

**Importante:** la prima esecuzione senza denoiser aveva restituito 0.5000 WER sul rumoroso originale e 0.5000 con calibrazione, mentre nella seconda esecuzione l'originale rumoroso ha fatto 0.1667. Potrebbero incidere differenze tra esecuzioni del riconoscitore CPU. **Non è legittimo attribuire un miglioramento robusto al denoiser da un solo campione/esecuzione**. Servono più clip, ripetizioni, diversi rapporti rumore/segnale, un modello italiano migliore e una baseline controllata.

Non è stato testato un sistema STT Multi-engine completo. Il denoiser non è stato abilitato sui satelliti esistenti; tutte le prove restano isolate su GitHub Actions.

## Conseguenza progettuale
Il futuro supervisore deve **valutare originale e versione filtrata**, scegliere sulla base di metriche reali/conferme e non applicare automaticamente il filtro a ogni frase: sul campione pulito non è cambiato il risultato. Inoltre non deve inventare parole perse nel segnale.
